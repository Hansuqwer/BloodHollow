// bh_server - BLOODHOLLOW headless authoritative world server (Phase 1-2).
// One process, one zone (Thornwall); zones-in-process per ADR-003.
// 20 Hz fixed tick. Protocol v0 (shared/protocol/messages.md).
// Sprint 5 (P2): stats/XP/levels, melee resolve, mobs from map spawners.
#include <algorithm>
#include <map>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <enet/enet.h>

#include "persist.h"
#include "loginlimit.h"
#include "protocol/messages_gen.h"
#include "sim/combat.h"
#include "sim/clock.h"
#include "sim/tick.h"
#include "command.h"
#include "world.h"

namespace bh::server {
namespace {

constexpr int kAoiRadius = 26;          // tiles, Chebyshev (architecture docs/03 section 4)
constexpr std::int64_t kTickMicros = 1000000 / sim::kTickHz;  // 50 ms
constexpr int kMaxChatLen = 160;
constexpr int kChatMinGapTicks = 10;    // 0.5 s

struct Session {
  ENetPeer* peer = nullptr;
  bool authed = false;
  bool inWorld = false;
  std::string user{};
  std::int64_t charRowId = 0;
  std::uint32_t entityId = 0;
  std::deque<Command> cmdq{};
  std::unordered_set<std::uint32_t> interest{};
  sim::Tick lastChatTick = -kChatMinGapTicks;
};

struct Server {
  // --bless user=item:qty,... (smoke-test only, empty by default)
  std::map<std::string, std::string> bless;

  ENetHost* host = nullptr;
  World world{};
  Db db{};
  std::unordered_map<ENetPeer*, Session> sessions{};
  sim::Tick tick = 0;
  std::uint16_t mapId = 1;
  // world journal (M2 gate): record every world-mutating interaction + hashes
  std::string recordWorldPath{};
  std::string replayWorldPath{};
  FILE* journal = nullptr;
  std::unordered_map<ENetPeer*, std::uint32_t> loginIndexPerPeer{};
  std::vector<std::string> loginOrder{};  // name per index (stable)
  // soak metrics
  std::vector<std::int64_t> tickMicros{};
  std::int64_t startEpochSec = 0;
  std::int64_t soakSecs = 0;
  double p99BudgetMs = 10.0;
  std::uint64_t packetsIn = 0, packetsOut = 0;
  // T-109: auth hardening (review §3.5) — per-source-IP login/registration
  // throttle + registration gate (--no-register). Limiter policy lives in
  // loginlimit.h (pure logic, unit-tested); these are its wiring points.
  LoginLimiter loginLimiter{};
  bool allowRegister = true;
};

// Monotonic milliseconds for the login limiter (never wall-clock: NTP steps
// must not shrink or stretch a lockout).
std::int64_t steadyNowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void pushOwnStats(Server& s, Session& sess);
void pushInventory(Server& s, Session& sess);

void send(ENetPeer* peer, const std::vector<std::uint8_t>& bytes, Server& s) {
  ENetPacket* p = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, p);
  s.packetsOut++;
}

template <typename Msg>
void sendMsg(ENetPeer* peer, const Msg& m, Server& s) {
  send(peer, proto::pack(m), s);
}

// Journal epoch: bump when the SIM semantics change under old journals
// (whitening/moral split in S15 = epoch 3; kit sidecars = 2; pre-K = 1;
// S22 T-034b mob retune = 6; T-068 barricade relocation = 7;
// T-069 fence + one-Marta spawn fix = 8; T-070 curse + confessor = 9;
// T-071 torch/lantern + nightOnly spawner = 10; T-073 guards + wanted = 11;
// T-079 refine rows +4..+7 = 12; T-091 Gravemother slam = 13;
// T-094 Thornwall NPC posts = 14; T-101 Old Maw = 15; T-102 Red Widow = 16;
// T-103 Cantor Vex = 17; T-107 worldHash widened to economy/progression = 18;
// T-104 review fixes + T-111 fixes = 19 (both lineages, independently
// numbered); T-115 reconciliation re-bumps: 20; T-120 weapon-skill
// persistence: 21.
// Replay refuses non-matching epoch journals instead of lying with them.
constexpr int kJournalEpoch = 21;  // T-120: sword_skill + swing_lands persisted,
                                   // journal l-line now carries skill. Fresh
                                   // gate leg: logs/t120.bwj

// ---- world journal record helpers (M2) ------------------------------------
void journalTickHash(Server& s) {
  if (s.journal == nullptr) return;
  if (std::getenv("BH_DUMP_ENTS") != nullptr) {
    for (const auto& ent : s.world.entities()) {
      std::fprintf(stderr,
                   "[live-dump] t=%lld id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercy=%u karma=%d atk=%u psz=%zu ch=%lld sw=%lld mv=%d tgt=%d:%d\n",
                   static_cast<long long>(s.tick), ent.id, static_cast<int>(ent.kind),
                   ent.wireKind, ent.zoneId, ent.walker.x, ent.walker.y,
                   ent.hp, ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma,
                   ent.attackTarget, ent.path.size(),
                   static_cast<long long>(ent.lastChaseTick), static_cast<long long>(ent.lastSwingTick),
                   ent.walker.moving ? 1 : 0, ent.walker.target.x, ent.walker.target.y);
    }
  }
  if (std::getenv("BH_DUMP_ENTS") != nullptr) {
    // T-049x probes: rng stream fingerprint + per-player progression/gear
    // fingerprint at hash cadence (pairs with [replay-rng]/[replay-ply]).
    std::fprintf(stderr, "[live-rng] t=%lld state=%016llx\n",
                 static_cast<long long>(s.tick),
                 static_cast<unsigned long long>(s.world.rng().stateFingerprint()));
    for (const auto& ent : s.world.entities()) {
      if (ent.kind != EntityKind::kPlayer) continue;
      std::fprintf(stderr,
                   "[live-ply] t=%lld id=%u name=%s lvl=%u xp=%u gold=%u "
                   "hp=%u/%u skill=%u inv=%s\n",
                   static_cast<long long>(s.tick), ent.id, ent.name.c_str(),
                   static_cast<unsigned>(ent.level), ent.xp, ent.gold, ent.hp,
                   ent.hpMax, static_cast<unsigned>(ent.swordSkill),
                   canonicalInvBlob(ent.inv).c_str());
    }
  }
  std::fprintf(s.journal, "h %lld %016llx\n", static_cast<long long>(s.tick),
               static_cast<unsigned long long>(s.world.worldHash()));
}

void journalLogin(Server& s, const Session& sess, const CharacterRow& row,
                  const Entity& e) {
  if (s.journal == nullptr) return;
  const std::uint32_t idx = static_cast<std::uint32_t>(s.loginOrder.size());
  s.loginIndexPerPeer[sess.peer] = idx;
  s.loginOrder.push_back(row.name);
  const sim::TilePos t = e.walker.tile();
  std::fprintf(s.journal,
               // v3 (T-120): zoneId column after (x,y) (T-036) + swordSkill +
               // swingLands before inv — persisted progression must replay.
               "l %lld %u %s %d %d %u %d %u %u %u %u %u %u %u %d %u %lld %s\n",
               static_cast<long long>(s.tick + 1), idx, row.name.c_str(), t.x, t.y,
               static_cast<unsigned>(e.zoneId),
               static_cast<unsigned>(row.level), static_cast<unsigned>(row.xp),
               static_cast<unsigned>(row.str), static_cast<unsigned>(row.vit),
               static_cast<unsigned>(row.dex), static_cast<unsigned>(row.statPoints),
               static_cast<unsigned>(row.gold),
               static_cast<unsigned>(row.anvilMercy), row.karma,
               static_cast<unsigned>(row.swordSkill),
               static_cast<long long>(row.swingLands),
               row.invBlob.empty() ? "-" : row.invBlob.c_str());
  // T-053: kit rides as a v2.1 sidecar line so pre-kit journals still parse.
  std::fprintf(s.journal, "k %lld %u %u\n",
               static_cast<long long>(s.tick + 1), idx,
               static_cast<unsigned>(row.classId > 0 ? row.classId : 1));
}

void journalDisconnect(Server& s, const Session& sess) {
  if (s.journal == nullptr) return;
  const auto it = s.loginIndexPerPeer.find(sess.peer);
  if (it == s.loginIndexPerPeer.end()) return;
  // pump-phase event: effect takes place BEFORE the next tick runs, so tag the
  // first tick where the world differs (replay applies before that world tick)
  std::fprintf(s.journal, "d %lld %u\n", static_cast<long long>(s.tick + 1),
               it->second);
}

void journalCommand(Server& s, const Session& sess, const Command& c) {
  if (s.journal == nullptr) return;
  const auto it = s.loginIndexPerPeer.find(sess.peer);
  if (it == s.loginIndexPerPeer.end()) return;
  // world-mutating kinds only; chat/ping carry no sim effect
  if (c.kind == Command::kChat || c.kind == Command::kPing) return;
  std::fprintf(s.journal, "c %lld %u %d %d %d %u\n",
               static_cast<long long>(s.tick), it->second, static_cast<int>(c.kind),
               c.a, c.b, static_cast<unsigned>(c.channel));
}

std::string sanitizeChat(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (const unsigned char c : in) {
    if (c >= 32 && c <= 126) out.push_back(static_cast<char>(c));
    if (out.size() >= kMaxChatLen) break;
  }
  // strip leading/trailing spaces
  const auto a = out.find_first_not_of(' ');
  const auto b = out.find_last_not_of(' ');
  if (a == std::string::npos) return "";
  return out.substr(a, b - a + 1);
}

void broadcastChat(Server& s, std::uint8_t channel, const std::string& from,
                   const std::string& text) {
  proto::ChatMsg msg;
  msg.channel = channel;
  msg.from = from;
  msg.text = text;
  const auto bytes = proto::pack(msg);
  if (channel == 0) {
    // say: AoI around the speaker
    const Entity* speaker = nullptr;
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld && kv.second.user == from) {
        speaker = s.world.find(kv.second.entityId);
        break;
      }
    }
    if (speaker == nullptr) return;
    const sim::TilePos sp = speaker->walker.tile();
    const auto ids = s.world.queryAoi(speaker->zoneId, sp.x, sp.y, kAoiRadius);
    const std::unordered_set<std::uint32_t> aoi(ids.begin(), ids.end());
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld && aoi.count(kv.second.entityId) > 0) {
        send(kv.first, bytes, s);
      }
    }
  } else {
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld) send(kv.first, bytes, s);
    }
  }
}

void dropSession(Server& s, Session& sess) {
  if (sess.inWorld) {
    const Entity* e = s.world.find(sess.entityId);
    if (e != nullptr && e->charRowId != 0) {
      const sim::TilePos p = e->walker.tile();
      s.db.savePosition(e->charRowId, e->zoneId, p.x, p.y);  // T-039: zone travels
      {
        // T-049x: canonical 7-field serializer shared with the probes
        const std::string blob = canonicalInvBlob(e->inv);
        s.db.saveProgress(e->charRowId, e->level, e->xp, e->str, e->vit, e->dex,
                          e->statPoints, static_cast<int>(e->gold), blob,
                          e->anvilMercyMask, e->karma, e->classId,
                          static_cast<int>(e->swordSkill),
                          static_cast<std::int64_t>(e->swingLands));
      }
      std::printf("[net] %-16s saved at (%d,%d)\n", e->name.c_str(), p.x, p.y);
    }
    s.world.despawn(sess.entityId);
    broadcastChat(s, 2, "", sess.user + " has left the world.");
    const sim::Tick now = s.tick;
    (void)now;
  }
  s.sessions.erase(sess.peer);
}

void handlePacket(Server& s, Session& sess, const proto::PacketView& pv) {
  using namespace proto;
  switch (pv.id) {
    case kIdHello: {
      Hello h;
      if (!h.deserialize(pv.body)) return;
      if (h.protoVersion != kProtocolVersion) {
        LoginResult r;
        r.ok = 0;
        r.reason = 4;  // protocol mismatch
        sendMsg(sess.peer, r, s);
        enet_peer_disconnect_later(sess.peer, 0);
        return;
      }
      // T-109: throttle + registration gate BEFORE any DB work. Reason codes:
      // 5 = rate-limited / bad-credentials lockout, 6 = registration disabled.
      // (1=bad credentials, 2=invalid name, 3=db error, 4=protocol mismatch.)
      const std::uint32_t ip = sess.peer->address.host;
      const std::int64_t nowMs = steadyNowMs();
      if (s.loginLimiter.lockedOut(ip, nowMs) ||
          !s.loginLimiter.allowLogin(ip, nowMs)) {
        LoginResult r;
        r.ok = 0;
        r.reason = 5;
        sendMsg(sess.peer, r, s);
        return;
      }
      bool acctExists = false;
      std::string acctErr;
      if (!s.db.accountExists(h.username, &acctExists, &acctErr)) {
        LoginResult r;
        r.ok = 0;
        r.reason = 3;  // server/db error
        sendMsg(sess.peer, r, s);
        return;
      }
      if (!acctExists &&
          (!s.allowRegister || !s.loginLimiter.allowRegister(ip, nowMs))) {
        LoginResult r;
        r.ok = 0;
        r.reason = s.allowRegister ? 5 : 6;
        sendMsg(sess.peer, r, s);
        return;
      }
      CharacterRow row;
      std::uint8_t reason = 0;
      std::string err;
      if (!s.db.loginOrCreate(h.username, h.password, &row, &reason, &err)) {
        if (reason == 1) s.loginLimiter.noteFailure(ip, nowMs);  // bad creds
        LoginResult r;
        r.ok = 0;
        r.reason = reason;
        sendMsg(sess.peer, r, s);
        return;
      }
      s.loginLimiter.noteSuccess(ip, nowMs);
      LoginResult r;
      r.ok = 1;
      r.reason = 0;
      sendMsg(sess.peer, r, s);

      sess.authed = true;
      sess.inWorld = true;
      sess.user = row.name;
      sess.charRowId = row.id;

      std::optional<sim::TilePos> at;
      if (row.x != 0 || row.y != 0) at = sim::TilePos{row.x, row.y};
      const std::uint16_t loginZone =
          static_cast<std::uint16_t>(row.mapId > 0 ? row.mapId : 1);
      Entity& e = s.world.spawn(row.name, row.id, at, loginZone,
                                static_cast<std::uint8_t>(row.classId));
      sess.entityId = e.id;
      journalLogin(s, sess, row, e);
      {
        // apply persisted progression (schema v2..v11)
        Entity* pe = s.world.find(e.id);
        pe->level = static_cast<std::uint8_t>(row.level < 1 ? 1 : (row.level > 25 ? 25 : row.level));
        pe->xp = static_cast<std::uint32_t>(row.xp < 0 ? 0 : row.xp);
        pe->str = static_cast<std::uint8_t>(row.str);
        pe->vit = static_cast<std::uint8_t>(row.vit);
        pe->dex = static_cast<std::uint8_t>(row.dex);
        pe->statPoints = static_cast<std::uint8_t>(row.statPoints);
        pe->gold = static_cast<std::uint32_t>(row.gold < 0 ? 0 : row.gold);
        // hpMax follows level/VIT; heal in full on login
        pe->hpMax = sim::playerHpMax(pe->level, pe->vit);
        pe->hp = pe->hpMax;
        // inventory blob "iid:qty:equipped:aura:durability:affix:refine;..."
        // T-049x: one shared grammar with the replay path (parseInvBlob),
        // slots appended in blob order — nothing laundered on relog.
        if (!row.invBlob.empty()) parseInvBlob(row.invBlob, pe->inv);
        pe->anvilMercyMask = static_cast<std::uint32_t>(row.anvilMercy);
        pe->karma = row.karma;
        // T-120: weapon-skill persistence (sword_skill + swing_lands)
        pe->swordSkill = static_cast<std::uint8_t>(row.swordSkill < 0 ? 0 : (row.swordSkill > 100 ? 100 : row.swordSkill));
        pe->swingLands = static_cast<std::uint32_t>(row.swingLands < 0 ? 0 : row.swingLands);
        // if lands present but skill 0 (old row with lands >0), recompute skill
        if (pe->swingLands > 0 && pe->swordSkill == 0) {
          pe->swordSkill = static_cast<std::uint8_t>(pe->swingLands / 25u);
        }
        const auto bit = s.bless.find(row.name);
        if (bit != s.bless.end()) {
          // parse "id:qty,id:qty"
          size_t bpos = 0;
          while (bpos < bit->second.size()) {
            const size_t bend = bit->second.find(',', bpos);
            const std::string pr = bit->second.substr(
                bpos, bend == std::string::npos ? std::string::npos : bend - bpos);
            bpos = (bend == std::string::npos) ? bit->second.size() : bend + 1;
            const size_t colon = pr.find(':');
            if (colon == std::string::npos) continue;
            if (pr.substr(0, colon) == "skill") {
              const std::uint32_t sk =
                  static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
              // combat derives swordSkill from swingLands/kSkillLandsPerPoint: seed the counter
              pe->swingLands = sk * 20u;
              pe->swordSkill = sk;
              continue;
            }
            if (pr.substr(0, colon) == "gold") {
              pe->gold = static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
              continue;
            }
            s.world.debugGive(*pe,
                              static_cast<std::uint32_t>(std::stoul(pr.substr(0, colon))),
                              static_cast<std::uint16_t>(std::stoul(pr.substr(colon + 1))));
          }
          std::printf("[bless] %s <- %s (skill now %u, gold %u)\n", row.name.c_str(),
                      bit->second.c_str(), static_cast<unsigned>(pe->swordSkill),
                      static_cast<unsigned>(pe->gold));
          for (const auto& sl : pe->inv)
            std::printf("[bless]   inv %u:%u eq=%u aura=%u\n", sl.itemId, sl.qty,
                        sl.equipped ? 1u : 0u, static_cast<unsigned>(sl.aura));
          if (s.journal != nullptr) {
            std::fprintf(s.journal, "b %lld %s %s\n",
                         static_cast<long long>(s.tick + 1), row.name.c_str(),
                         bit->second.c_str());
          }
        }
        // T-049x probe: post-application login fingerprint (pairs with
        // [replay-login] — the relog-launderer canary).
        if (std::getenv("BH_DUMP_ENTS") != nullptr) {
          std::fprintf(stderr,
                       "[live-login] name=%s lvl=%u xp=%u gold=%u hp=%u/%u "
                       "skill=%u mercy=%u karma=%d inv=%s\n",
                       row.name.c_str(), static_cast<unsigned>(pe->level), pe->xp,
                       pe->gold, pe->hp, pe->hpMax,
                       static_cast<unsigned>(pe->swordSkill), pe->anvilMercyMask,
                       pe->karma, canonicalInvBlob(pe->inv).c_str());
        }
      }

      Welcome w;
      w.entityId = e.id;
      w.mapId = loginZone;
      w.x = e.walker.x;
      w.y = e.walker.y;
      w.tick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
      w.hourCenti = static_cast<std::uint32_t>(sim::hourAt(s.tick) * 100.0f);
      sendMsg(sess.peer, w, s);
      pushOwnStats(s, sess);
      pushInventory(s, sess);
      std::printf("[net] %-16s entered the world (entity %u, online %zu)\n",
                  row.name.c_str(), sess.entityId, s.sessions.size());
      broadcastChat(s, 2, "", row.name + " has entered Thornwall.");
      break;
    }
    case kIdInputPath: {
      InputPath m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kPath;
      c.a = m.goalX;
      c.b = m.goalY;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdInputStep: {
      InputStep m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kStep;
      c.a = m.dx;
      c.b = m.dy;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdChatSend: {
      ChatSend m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      if (!m.text.empty() && m.text[0] == '/') {  // party verbs (era commands)
        Command c;
        bool okCmd = true;
        if (m.text.rfind("/invite ", 0) == 0) {
          const std::string nm = m.text.substr(8);
          okCmd = false;
          for (const auto& e : s.world.entities()) {
            if (e.kind == EntityKind::kPlayer && e.name == nm) {
              c.kind = Command::kPartyInvite;
              c.a = static_cast<std::int32_t>(e.id);
              okCmd = true;
              break;
            }
          }
        } else if (m.text == "/accept") { c.kind = Command::kPartyAccept; }
        else if (m.text == "/leave") { c.kind = Command::kPartyLeave; }
        else if (m.text.rfind("/kick ", 0) == 0) {
          const std::string nm = m.text.substr(6);
          okCmd = false;
          for (const auto& e : s.world.entities()) {
            if (e.kind == EntityKind::kPlayer && e.name == nm) {
              c.kind = Command::kPartyKick;
              c.a = static_cast<std::int32_t>(e.id);
              okCmd = true;
              break;
            }
          }
        } else if (m.text.rfind("/duel ", 0) == 0) {  // T-056 offer/accept
          const std::string nm = m.text.substr(6);
          okCmd = false;
          for (const auto& e : s.world.entities()) {
            if (e.kind == EntityKind::kPlayer && e.name == nm) {
              c.kind = Command::kDuel;
              c.a = static_cast<std::int32_t>(e.id);
              okCmd = true;
              break;
            }
          }
        } else if (m.text == "/forfeit") { c.kind = Command::kForfeit; }
        else if (m.text == "/repair") { c.kind = Command::kRepair; }
        else if (m.text == "/confess") { c.kind = Command::kConfess; }
        else if (m.text == "/repent") { c.kind = Command::kRepent; }
        else if (m.text.rfind("/refine ", 0) == 0) {  // T-060 anvil upgrade
          // T-104: throw-free parse (was std::stoi on client digits — any
          // all-digit argument wider than int aborted the server).
          std::int32_t slot = 0;
          okCmd = parseRefineArg(m.text.substr(8), slot);
          if (okCmd) {
            c.kind = Command::kRefine;
            c.a = slot;
          }
        }
        else if (m.text.rfind("/kit ", 0) == 0) {  // T-053 one-time swear
          const std::string k = m.text.substr(5);
          std::uint8_t kit = 0;
          if (k == "ravager") kit = 1;
          else if (k == "gravecaller") kit = 2;
          else if (k == "cultist" || k == "choir") kit = 3;
          if (kit != 0) {
            c.kind = Command::kKitChoose;
            c.a = kit;
          } else {
            okCmd = false;  // unknown oath name: say it aloud, era-right
          }
        } else { okCmd = false; }
        if (okCmd && sess.cmdq.size() < 32) { sess.cmdq.push_back(std::move(c)); break; }
        // fall through: unknown/failed slash visible as an ordinary say
      }
      Command c;
      c.kind = Command::kChat;
      c.channel = m.channel;
      c.text = std::move(m.text);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdAttackRequest: {
      AttackRequest m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kAttack;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdStatAssign: {
      StatAssign m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kStat;
      c.channel = m.stat;  // reuse byte slot
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdUseItem: {
      UseItem m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kUseItem;
      c.channel = m.slot;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdToggleEquip: {
      ToggleEquip m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kEquip;
      c.channel = m.slot;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdSkillUse: {
      SkillUse m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kSkill;
      c.channel = m.skill;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdBuyRequest: {
      BuyRequest m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kBuy;
      c.a = static_cast<std::int32_t>(m.itemId);
      c.b = m.qty;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdSellJunk: {
      SellJunk m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kSellJunk;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOpen: {
      TradeOpen m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeOpen;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOfferItem: {
      TradeOfferItem m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeItem;
      c.a = static_cast<std::int32_t>(m.itemId);
      c.b = m.qty;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOfferGold: {
      TradeOfferGold m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeGold;
      c.a = static_cast<std::int32_t>(m.gold);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeCommit: {
      TradeCommit m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeCommit;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeCancel: {
      TradeCancel m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeCancel;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdAnvilOp: {
      AnvilOp m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kAnvil;
      c.channel = m.tier;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdPartyInvite: {
      PartyInvite m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      for (const auto& e : s.world.entities()) {  // exact-name resolve (era whispers)
        if (e.kind == EntityKind::kPlayer && e.name == m.name) {
          Command c;
          c.kind = Command::kPartyInvite;      // id pinned here; journal replays ids
          c.a = static_cast<std::int32_t>(e.id);
          if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
          break;
        }
      }
      break;
    }
    case kIdPartyAccept: {
      PartyAccept m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kPartyAccept;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdPartyLeave: {
      PartyLeave m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kPartyLeave;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdPartyKick: {
      PartyKick m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kPartyKick;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdPing: {
      Ping m;
      if (!m.deserialize(pv.body)) return;
      Pong p;
      p.clientTimeMs = m.clientTimeMs;
      p.serverTick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
      sendMsg(sess.peer, p, s);
      break;
    }
    default:
      break;
  }
}

void processCommand(Server& s, Session& sess, const Command& c) {
  journalCommand(s, sess, c);
  if (c.kind == Command::kChat) {
    if (s.tick - sess.lastChatTick < kChatMinGapTicks) return;  // rate limit
    sess.lastChatTick = s.tick;
    const std::string text = sanitizeChat(c.text);
    if (text.empty()) return;
    broadcastChat(s, c.channel == 0 ? 0 : 1, sess.user, text);
    return;
  }
  if (c.kind == Command::kPing) return;
  Entity* e = s.world.find(sess.entityId);
  if (e == nullptr) return;
  applyWorldCommand(s.world, *e, c);  // single source of truth (T-049)
}

void pushInventory(Server& s, Session& sess) {
  const Entity* e = s.world.find(sess.entityId);
  if (e == nullptr) return;
  proto::InventoryReset r;
  r.count = static_cast<std::uint8_t>(e->inv.size());
  sendMsg(sess.peer, r, s);
  for (size_t i = 0; i < e->inv.size(); ++i) {
    proto::ItemSlot m;
    m.slot = static_cast<std::uint8_t>(i);
    m.itemId = e->inv[i].itemId;
    m.qty = e->inv[i].qty;
    m.equipped = e->inv[i].equipped ? 1 : 0;
    m.aura = e->inv[i].aura;
    m.durability = e->inv[i].durability;  // T-058
    m.affix = e->inv[i].affix;            // T-059
    m.refine = e->inv[i].refine;          // T-060
    sendMsg(sess.peer, m, s);
  }
}

void pushOwnStats(Server& s, Session& sess) {
  const Entity* e = s.world.find(sess.entityId);
  if (e == nullptr || e->kind != EntityKind::kPlayer) return;
  proto::OwnStats m;
  m.level = e->level;
  m.xp = e->xp;
  m.xpNext = sim::xpNext(e->level);
  m.statPoints = e->statPoints;
  m.str = e->str;
  m.vit = e->vit;
  m.dex = e->dex;
  m.intg = e->intg;
  m.mag = e->mag;
  m.swordSkill = e->swordSkill;
  m.gold = e->gold;
  m.karma = e->karma;
  m.classId = e->classId;  // T-053 kit + kit-state readout
  m.mp = e->mp;
  m.mpMax = e->mpMax == 0 ? 1 : e->mpMax;
  auto left = [&](sim::Tick until) -> std::uint16_t {
    const sim::Tick t = s.world.tickCount();
    if (until < 0 || t >= until) return 0;
    const sim::Tick d = until - t;
    return d > 0xFFFF ? std::uint16_t(0xFFFF) : static_cast<std::uint16_t>(d);
  };
  m.blessTicksLeft = left(e->blessUntil);
  m.ironskinTicksLeft = left(e->ironskinUntil);
  m.curseTicksLeft = left(e->curseUntil);  // T-070 thin blood readout
  sendMsg(sess.peer, m, s);
}

// Distribute world events after each sim tick.
// Party roster push (T-050): PartyReset + PartyMember rows, inventory-style.
void pushPartyRoster(Server& s, Session& sess) {
  Entity* me = s.world.find(sess.entityId);
  const World::Party* p = me != nullptr ? s.world.partyOf(me->id) : nullptr;
  proto::PartyReset r;
  r.partyId = p != nullptr ? p->id : 0;
  r.leaderId = p != nullptr ? p->leaderId : 0;
  r.count = p != nullptr ? static_cast<std::uint8_t>(p->members.size()) : 0;
  sendMsg(sess.peer, r, s);
  if (p == nullptr) return;
  for (const std::uint32_t mid : p->members) {
    const Entity* m = s.world.find(mid);
    if (m == nullptr) continue;
    proto::PartyMember pm;
    pm.entityId = m->id;
    pm.name = m->name;
    pm.level = m->level;
    pm.hp = m->hp;
    pm.hpMax = m->hpMax;
    pm.zoneId = m->zoneId;
    sendMsg(sess.peer, pm, s);
  }
}

void distributeEvents(Server& s) {
  for (const WorldEvent& ev : s.world.events()) {
    if (ev.chatCh == 255 && !ev.chatText.empty()) {
      proto::ChatMsg m;
      m.channel = 2;
      m.from = "";
      m.text = ev.chatText;
      const auto bytes = proto::pack(m);
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          send(kv.first, bytes, s);
        }
      }
    } else if (ev.chatCh != 0 && !ev.chatText.empty()) {
      broadcastChat(s, ev.chatCh, "", ev.chatText);
    }
    if (ev.zoneChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        Session& sess = kv.second;
        if (!sess.inWorld || sess.entityId != ev.aboutId) continue;
        const Entity* e = s.world.find(ev.aboutId);
        if (e == nullptr) continue;
        sess.interest.clear();  // force full respawn burst for the new zone
        proto::Welcome w;
        w.entityId = e->id;
        w.mapId = e->zoneId;
        w.x = e->walker.x;
        w.y = e->walker.y;
        w.tick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
        w.hourCenti = static_cast<std::uint32_t>(sim::hourAt(s.tick) * 100.0f);
        sendMsg(sess.peer, w, s);
        pushOwnStats(s, sess);
        pushInventory(s, sess);
        s.db.savePosition(e->charRowId, e->zoneId, e->walker.tile().x, e->walker.tile().y);
        std::printf("[zone] %s -> map %u at (%d,%d)\n", e->name.c_str(),
                    static_cast<unsigned>(e->zoneId), e->walker.tile().x,
                    e->walker.tile().y);
        std::fflush(stdout);
      }
    }
    if (ev.bandChanged && ev.aboutId != 0) {  // T-057: re-spawn to all online
      const Entity* e = s.world.find(ev.aboutId);
      if (e != nullptr) {
        for (auto& kv : s.sessions) {
          if (!kv.second.inWorld) continue;
          proto::EntitySpawn m;
          m.id = e->id;
          m.kind = e->wireKind;
          m.dir = static_cast<std::uint8_t>(e->walker.dir);
          m.x = e->walker.x;
          m.y = e->walker.y;
          m.hp = e->hp;
          m.hpMax = e->hpMax;
          m.level = e->level;
          m.name = e->name;
          m.karmaBand = World::karmaBandOf(e->karma);
          m.light = e->lightRadius;  // T-071 night light
          m.glowTier = s.world.equippedGlowTier(*e);  // T-092 refine glow
          sendMsg(kv.first, m, s);
        }
      }
    }
    if (ev.partyChanged) {
      if (ev.target != 0) {
        const World::Party* pp = nullptr;
        for (const auto& q : s.world.parties())
          if (q.id == ev.target) { pp = &q; break; }
        if (pp != nullptr) {
          for (const std::uint32_t mid : pp->members)
            for (auto& kv : s.sessions)
              if (kv.second.inWorld && kv.second.entityId == mid) pushPartyRoster(s, kv.second);
        }
      }
      for (auto& kv : s.sessions)  // aboutId refresh (covers leave/kick wipe too)
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) pushPartyRoster(s, kv.second);
    }
    if (ev.invChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          pushInventory(s, kv.second);
        }
      }
    }
    if (ev.statsChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          pushOwnStats(s, kv.second);
        }
      }
    }
    if (ev.attacker != 0 || ev.target != 0) {
      // combat pulse: to everyone whose AoI contains either endpoint
      proto::CombatEvent m;
      m.attackerId = ev.attacker;
      m.targetId = ev.target;
      m.kind = ev.kind;
      m.amount = ev.amount;
      const auto bytes = proto::pack(m);
      for (auto& kv : s.sessions) {
        Session& viewer = kv.second;
        if (!viewer.inWorld) continue;
        if (viewer.interest.count(ev.attacker) > 0 ||
            viewer.interest.count(ev.target) > 0) {
          send(viewer.peer, bytes, s);
        }
      }
    }
  }
}

void tickServer(Server& s) {
  const auto t0 = std::chrono::steady_clock::now();
  ++s.tick;

  // 1) one queued command per session per tick (fair-share intents)
  for (auto& kv : s.sessions) {
    if (!kv.second.cmdq.empty()) {
      processCommand(s, kv.second, kv.second.cmdq.front());
      kv.second.cmdq.pop_front();
    }
  }

  // 1.5) events produced by commands land before tick() wipes the queue
  distributeEvents(s);

  // 2) simulate (movement + combat + respawns)
  s.world.tick();
  distributeEvents(s);

  // 3) AoI deltas per session
  for (auto& kv : s.sessions) {
    Session& sess = kv.second;
    if (!sess.inWorld) continue;
    const Entity* me = s.world.find(sess.entityId);
    if (me == nullptr) continue;
    const sim::TilePos p = me->walker.tile();
    const auto ids = s.world.queryAoi(me->zoneId, p.x, p.y, kAoiRadius);
    std::unordered_set<std::uint32_t> now(ids.begin(), ids.end());

    for (const std::uint32_t id : now) {
      const Entity* e = s.world.find(id);
      if (e == nullptr) continue;
      if (sess.interest.count(id) == 0) {
        proto::EntitySpawn m;
        m.id = e->id;
        m.kind = e->wireKind;
        m.dir = static_cast<std::uint8_t>(e->walker.dir);
        m.x = e->walker.x;
        m.y = e->walker.y;
        m.hp = e->hp;
        m.hpMax = e->hpMax;
        m.level = e->kind == EntityKind::kPlayer ? e->level : e->mobLevel;
        m.name = e->name;
        m.karmaBand = e->kind == EntityKind::kPlayer
                          ? World::karmaBandOf(e->karma)
                          : std::uint8_t(1);  // mobs: neutral band
        m.light = e->lightRadius;  // T-071 night light
        m.glowTier = s.world.equippedGlowTier(*e);  // T-092 refine glow
        sendMsg(sess.peer, m, s);
      }
      proto::EntityDelta d;
      d.id = e->id;
      d.x = e->walker.x;
      d.y = e->walker.y;
      d.dir = static_cast<std::uint8_t>(e->walker.dir);
      d.moving = e->walker.moving ? 1 : 0;
      d.hp = e->hp;
      d.light = e->lightRadius;  // T-071 (torch expiry/toggle rides the delta)
      d.glowTier = s.world.equippedGlowTier(*e);  // T-092 (refine swaps ride too)
      sendMsg(sess.peer, d, s);
    }
    for (const std::uint32_t id : sess.interest) {
      if (now.count(id) == 0) {
        proto::EntityDespawn dm;
        dm.id = id;
        sendMsg(sess.peer, dm, s);
      }
    }
    sess.interest = std::move(now);
  }

  // 4) periodic stats + soak bookkeeping
  const auto t1 = std::chrono::steady_clock::now();
  s.tickMicros.push_back(
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
  if (s.tickMicros.size() > 600) s.tickMicros.erase(s.tickMicros.begin());

  if (s.tick % 20 == 0) {
    if (!s.world.parties().empty()) {  // party-frame hp/level refresh, 1 Hz
      for (auto& kv : s.sessions)
        if (kv.second.inWorld) pushPartyRoster(s, kv.second);
    }
    std::vector<std::int64_t> sorted = s.tickMicros;
    std::sort(sorted.begin(), sorted.end());
    const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
    proto::ServerStats st;
    st.online = static_cast<std::uint32_t>(s.sessions.size());
    st.tickMicrosP99 = static_cast<std::uint32_t>(p99);
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld) sendMsg(kv.first, st, s);
    }
  }

  {
    static std::int64_t cadence = -1;
    if (cadence < 0) {
      const char* e = std::getenv("BH_HASH_CADENCE");
      cadence = (e != nullptr && std::atoi(e) > 0) ? std::atoi(e) : 100;
    }
    if (s.journal != nullptr && s.tick % cadence == 0) journalTickHash(s);
  }

  if (s.tick % 600 == 0) {  // every 30 s
    std::vector<std::int64_t> sorted = s.tickMicros;
    std::sort(sorted.begin(), sorted.end());
    const std::int64_t p50 = sorted.empty() ? 0 : sorted[sorted.size() / 2];
    const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
    std::printf(
        "[soak] tick=%lld online=%zu entities=%zu tickUs p50=%lld p99=%lld worldHash=%016llx "
        "pkts=%zu/%zu\n",
        static_cast<long long>(s.tick), s.sessions.size(), s.world.count(),
        static_cast<long long>(p50), static_cast<long long>(p99),
        static_cast<unsigned long long>(s.world.worldHash()),
        static_cast<size_t>(s.packetsIn), static_cast<size_t>(s.packetsOut));
    std::fflush(stdout);
  }
}

// ---- replay-world: deterministic verification runner (M2 wipe repro) ------
// Reads a .bwj journal: l=login(tick,name,idx,x,y,level,xp,str,vit,dex,sp,gold,inv);
// c=command(tick,loginIdx,kind,a,b,channel); h=tick hash markers every 100.
// Replays entity creation + world commands on a fresh world (fixed seed), then
// verifies every hash marker. Exit 0 = perfect replay (wipe+all), 3 = mismatch.
int runReplayWorld(const std::string& path, const std::string& mapPath) {
  {
    FILE* pf = std::fopen(path.c_str(), "r");
    if (pf != nullptr) {
      char vline[32];
      if (std::fgets(vline, sizeof vline, pf) != nullptr && vline[0] == 'v') {
        int ep = 1;
        std::sscanf(vline, "v %d", &ep);
        if (ep != kJournalEpoch) {
          std::fprintf(stderr,
              "[replay] journal epoch %d vs build epoch %d — sim semantics "
              "changed since; record a fresh gate leg (old leg retained as "
              "history)\n", ep, kJournalEpoch);
          std::fclose(pf);
          return 4;
        }
      }
      std::fclose(pf);
    }
  }
  FILE* f = std::fopen(path.c_str(), "r");
  if (f == nullptr) {
    std::fprintf(stderr, "bh_server: cannot open journal %s\n", path.c_str());
    return 1;
  }
  std::string err;
  World world;
  if (!world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    std::fclose(f);
    return 1;
  }
  {  // mirror the server's boot set (any zone asset that's actually there)
    for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                     {3, "assets/maps/thornwall_crypt.bhmap"},
                                     {4, "assets/maps/bonehowl_mine.bhmap"},
                                     {5, "assets/maps/drowned_crypt.bhmap"}}) {
      std::string zerr;
      (void)world.loadZone(zid, zpath, &zerr);
    }
  }
  struct ReplayEnt {
    std::uint32_t id = 0;
  };
  std::vector<ReplayEnt> entities;
  struct QueuedLogin {
    sim::Tick tick;
    std::uint32_t idx;
    std::string name;
    int x, y;
    std::uint32_t zoneId = 1;  // v2 journals carry it; earlier = zone 1
    unsigned level, xp, st_, vit, dex, sp, gold;
    std::uint32_t mercyMask = 0;
    std::int32_t karma = 0;
    unsigned swordSkill = 0;       // v3 (T-120)
    std::int64_t swingLands = 0;   // v3 (T-120)
    std::string inv;
  };
  struct QueuedBless { sim::Tick tick; std::string name; std::string spec; };
  std::unordered_map<std::uint32_t, std::uint32_t> loginKits;  // k-lines (T-053)
  
  struct QueuedCmd {
    sim::Tick tick;
    std::uint32_t idx;
    int kind;
    std::int32_t a, b;
    std::uint32_t channel;
  };
  struct QueuedHash {
    sim::Tick tick;
    std::uint64_t expected;
  };
  std::vector<QueuedLogin> logins;
  std::vector<QueuedCmd> cmds;
  std::vector<QueuedHash> hashes;
  std::vector<QueuedBless> queuedBlesses;
  std::vector<QueuedCmd> drops;  // idx-only, kind = -1
  sim::Tick lastTick = 0;
  char line[1024];
  while (std::fgets(line, sizeof line, f) != nullptr) {
    if (line[0] == 'l') {
      // l tick idx name x y zone level xp str vit dex sp gold mercy karma swordSkill swingLands inv (v3 T-120)
      char name[64], inv[768] = "-";
      long long tick;
      unsigned idx, level, xp, st_, vit, dex, sp, gold, mercy = 0, zone = 1, swordSkill = 0;
      long long swingLands = 0;
      int x, y, karma = 0;
      int n = std::sscanf(line, "l %lld %u %63s %d %d %u %u %u %u %u %u %u %u %u %d %u %lld %767s",
                          &tick, &idx, name, &x, &y, &zone, &level, &xp, &st_, &vit,
                          &dex, &sp, &gold, &mercy, &karma, &swordSkill, &swingLands, inv);
      if (n != 18) {
        // try v2 (zone + mercy+karma, no skill)
        swordSkill = 0; swingLands = 0;
        n = std::sscanf(line, "l %lld %u %63s %d %d %u %u %u %u %u %u %u %u %u %d %767s",
                        &tick, &idx, name, &x, &y, &zone, &level, &xp, &st_, &vit,
                        &dex, &sp, &gold, &mercy, &karma, inv);
        if (n != 16) {  // legacy v1: no zone column
          zone = 1;
          n = std::sscanf(line, "l %lld %u %63s %d %d %u %u %u %u %u %u %u %u %d %767s",
                          &tick, &idx, name, &x, &y, &level, &xp, &st_, &vit,
                          &dex, &sp, &gold, &mercy, &karma, inv);
          if (n != 15 && n != 13) {
            std::fprintf(stderr, "[replay] malformed l-line: %s", line);
            std::fclose(f);
            return 2;
          }
          if (n == 13) { mercy = 0; karma = 0; }  // pre-v6 journals
        }
      }
      logins.push_back(QueuedLogin{static_cast<sim::Tick>(tick), idx, name, x, y, zone,
                                   level, xp, st_, vit, dex, sp, gold, mercy, karma,
                                   swordSkill, swingLands, inv});
      if (tick > lastTick) lastTick = tick;
    } else if (line[0] == 'k') {  // T-053 kit sidecar: k tick idx kitId
      long long ktick; unsigned kidx, kit;
      if (std::sscanf(line, "k %lld %u %u", &ktick, &kidx, &kit) == 3)
        loginKits[kidx] = kit;
    } else if (line[0] == 'b') {
      long long tick;
      char nm[64], spec[256];
      if (std::sscanf(line, "b %lld %63s %255s", &tick, nm, spec) == 3) {
        queuedBlesses.push_back(QueuedBless{static_cast<sim::Tick>(tick), nm, spec});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'c') {
      long long tick;
      unsigned idx, ch;
      int kind, a, b;
      if (std::sscanf(line, "c %lld %u %d %d %d %u", &tick, &idx, &kind, &a, &b, &ch) == 6) {
        cmds.push_back(QueuedCmd{static_cast<sim::Tick>(tick), idx, kind, a, b, ch});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'd') {
      long long tick;
      unsigned idx;
      if (std::sscanf(line, "d %lld %u", &tick, &idx) == 2) {
        drops.push_back(QueuedCmd{static_cast<sim::Tick>(tick), idx, -1, 0, 0, 0});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'h') {
      long long tick;
      unsigned long long h;
      char hex[40];
      // T-049x: a hash field that isn't exactly 16 hex digits is a TRUNCATED
      // line (ungraceful kill mid-write) — refuse to verify against garbage;
      // say so and stop instead of failing later with a phantom mismatch.
      if (std::sscanf(line, "h %lld %39s", &tick, hex) == 2) {
        if (std::strlen(hex) != 16 ||
            std::sscanf(hex, "%llx", &h) != 1) {
          std::fprintf(stderr,
                       "[replay] truncated hash line at tick %lld: '%s' — "
                       "journal tail cut mid-write?\n",
                       tick, line);
          std::fclose(f);
          return 4;
        }
        hashes.push_back(QueuedHash{static_cast<sim::Tick>(tick), h});
        if (tick > lastTick) lastTick = tick;
      }
    }
  }
  std::fclose(f);

  size_t ci = 0, hi = 0, li = 0, di = 0;
  std::int64_t checked = 0, bad = 0;
  auto applyLogin = [&](const QueuedLogin& L) {
    std::uint8_t kit = 1;  // pre-v2.1 journals default Ravager (freeze)
    if (const auto ki = loginKits.find(L.idx); ki != loginKits.end())
      kit = static_cast<std::uint8_t>(ki->second);
    Entity& e = world.spawn(L.name, 0, sim::TilePos{L.x, L.y},
                            static_cast<std::uint16_t>(L.zoneId), kit);
    Entity* pe = world.find(e.id);
    pe->level = static_cast<std::uint8_t>(L.level < 1 ? 1 : (L.level > 25 ? 25 : L.level));
    pe->xp = L.xp;
    pe->str = static_cast<std::uint8_t>(L.st_);
    pe->vit = static_cast<std::uint8_t>(L.vit);
    pe->dex = static_cast<std::uint8_t>(L.dex);
    pe->statPoints = static_cast<std::uint8_t>(L.sp);
    pe->gold = L.gold;
    pe->hpMax = sim::playerHpMax(pe->level, pe->vit);
    pe->hp = pe->hpMax;
    // T-049x: full 7-field grammar via the SHARED parser, slots appended in
    // blob order — the live login path verbatim. (The old 4-field sscanf +
    // debugGive lane laundered gear: dormant 0-durability items resurrected
    // to 100, affix/refine dropped, slot order scrambled by stacking.)
    if (L.inv != "-") parseInvBlob(L.inv, pe->inv);
    pe->anvilMercyMask = L.mercyMask;
    pe->karma = L.karma;
    // T-120: persisted skill (v3 journal)
    pe->swordSkill = static_cast<std::uint8_t>(L.swordSkill > 100 ? 100 : L.swordSkill);
    pe->swingLands = static_cast<std::uint32_t>(L.swingLands < 0 ? 0 : L.swingLands);
    if (pe->swingLands > 0 && pe->swordSkill == 0) {
      pe->swordSkill = static_cast<std::uint8_t>(pe->swingLands / 25u);
    }
    // replay bless grants (debug lane recorded as b-lines) — AFTER the
    // persisted blob, exactly like the live login order, so debugGive
    // stacking/appending lands identically on both sides.
    for (const auto& qb : queuedBlesses) {
      if (qb.name != L.name) continue;
      size_t bpos = 0;
      while (bpos < qb.spec.size()) {
        const size_t bend = qb.spec.find(',', bpos);
        const std::string pr = qb.spec.substr(
            bpos, bend == std::string::npos ? std::string::npos : bend - bpos);
        bpos = (bend == std::string::npos) ? qb.spec.size() : bend + 1;
        const size_t colon = pr.find(':');
        if (colon == std::string::npos) continue;
        if (pr.substr(0, colon) == "skill") {
          const std::uint32_t sk =
              static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
          pe->swingLands = sk * 20u;
          pe->swordSkill = sk;
          continue;
        }
        if (pr.substr(0, colon) == "gold") {
          pe->gold = static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
          continue;
        }
        world.debugGive(*pe,
                        static_cast<std::uint32_t>(std::stoul(pr.substr(0, colon))),
                        static_cast<std::uint16_t>(std::stoul(pr.substr(colon + 1))));
      }
    }
    // T-049x probe: post-application login fingerprint (pairs with
    // [live-login] — diff these two lanes to catch any future laundering).
    if (std::getenv("BH_DUMP_ENTS") != nullptr) {
      std::fprintf(stderr,
                   "[replay-login] name=%s lvl=%u xp=%u gold=%u hp=%u/%u "
                   "skill=%u mercy=%u karma=%d inv=%s\n",
                   pe->name.c_str(), static_cast<unsigned>(pe->level), pe->xp,
                   pe->gold, pe->hp, pe->hpMax,
                   static_cast<unsigned>(pe->swordSkill), pe->anvilMercyMask,
                   pe->karma, canonicalInvBlob(pe->inv).c_str());
    }
    entities.push_back(ReplayEnt{e.id});
  };
  // 1-based ticks match tickServer(): ++s.tick lands BEFORE cmds+world.tick()
  for (sim::Tick t = 1; t <= lastTick + 20; ++t) {
    while (li < logins.size() && logins[li].tick <= t) applyLogin(logins[li++]);
    while (di < drops.size() && drops[di].tick <= t) {
      if (drops[di].idx < entities.size()) world.despawn(entities[drops[di].idx].id);
      ++di;
    }
    while (ci < cmds.size() && cmds[ci].tick <= t) {
      const QueuedCmd& q = cmds[ci++];
      if (q.idx >= entities.size()) continue;
      Entity* e = world.find(entities[q.idx].id);
      if (e == nullptr) continue;
      // Journal lines carry the exact original Command fields; replay through
      // the same single application path as the live server (T-049).
      Command cmd;
      cmd.kind = static_cast<Command::Kind>(q.kind);
      cmd.a = q.a;
      cmd.b = q.b;
      cmd.channel = q.channel;
      applyWorldCommand(world, *e, cmd);
    }
    world.tick();
    if (std::getenv("BH_DUMP_ENTS") != nullptr) {
      for (const auto& ent : world.entities()) {
        std::fprintf(stderr,
                     "[replay-dump] t=%lld id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercymask=%u karma=%d atk=%u psz=%zu ch=%lld sw=%lld mv=%d tgt=%d:%d\n",
                     static_cast<long long>(t), ent.id, static_cast<int>(ent.kind),
                     ent.wireKind, ent.zoneId, ent.walker.x, ent.walker.y, ent.hp,
                     ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma,
                     ent.attackTarget, ent.path.size(),
                     static_cast<long long>(ent.lastChaseTick), static_cast<long long>(ent.lastSwingTick),
                     ent.walker.moving ? 1 : 0, ent.walker.target.x, ent.walker.target.y);
      }
      // T-049x probes (pair with [live-rng]/[live-ply]; live emits at hash
      // cadence, replay every tick — grep by tick to compare).
      std::fprintf(stderr, "[replay-rng] t=%lld state=%016llx\n",
                   static_cast<long long>(t),
                   static_cast<unsigned long long>(world.rng().stateFingerprint()));
      for (const auto& ent : world.entities()) {
        if (ent.kind != EntityKind::kPlayer) continue;
        std::fprintf(stderr,
                     "[replay-ply] t=%lld id=%u name=%s lvl=%u xp=%u gold=%u "
                     "hp=%u/%u skill=%u inv=%s\n",
                     static_cast<long long>(t), ent.id, ent.name.c_str(),
                     static_cast<unsigned>(ent.level), ent.xp, ent.gold, ent.hp,
                     ent.hpMax, static_cast<unsigned>(ent.swordSkill),
                     canonicalInvBlob(ent.inv).c_str());
      }
    }
    while (hi < hashes.size() && hashes[hi].tick <= t) {
      const QueuedHash& qh = hashes[hi++];
      ++checked;
      if (world.worldHash() != qh.expected) {
        ++bad;
        std::fprintf(stdout, "[replay] MISMATCH tick=%lld expected=%016llx got=%016llx\n",
                     static_cast<long long>(qh.tick),
                     static_cast<unsigned long long>(qh.expected),
                     static_cast<unsigned long long>(world.worldHash()));
        if (bad == 1) {
          
          for (const auto& ent : world.entities()) {
            {
              std::fprintf(stderr,
                           "[replay-dump] id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercymask=%u karma=%d\n",
                           ent.id, static_cast<int>(ent.kind), ent.wireKind, ent.zoneId,
                           ent.walker.x, ent.walker.y, ent.hp,
                           ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma);
            }
          }
          std::fprintf(stderr, "[replay-dump] total entities=%zu tickCnt=%llu\n",
                       world.entities().size(),
                       static_cast<unsigned long long>(world.tickCount()));
        }
      }
    }
  }
  std::printf("[replay] %s ticks=%lld sessionCmds=%zu hashes=%lld mismatches=%lld "
              "entities=%zu\n",
              bad == 0 ? "OK" : "FAIL", static_cast<long long>(lastTick), cmds.size(),
              static_cast<long long>(checked), static_cast<long long>(bad),
              world.count());
  return bad == 0 ? 0 : 3;
}

int run(int argc, char** argv) {
  std::string mapPath = "assets/maps/thornwall.bhmap";
  std::string dbPath = "var/bh_server.db";
  std::uint16_t port = 7777;
  Server s;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&](const char* dflt) -> std::string {
      return i + 1 < argc ? argv[++i] : dflt;
    };
    if (a == "--map") mapPath = next(mapPath.c_str());
    else if (a == "--db") dbPath = next(dbPath.c_str());
    else if (a == "--port") port = static_cast<std::uint16_t>(std::atoi(next("7777").c_str()));
    else if (a == "--soak-secs") s.soakSecs = std::atoll(next("0").c_str());
    else if (a == "--record-world") s.recordWorldPath = next("");
    else if (a == "--bless") {
      const std::string spec = next("");
      const size_t eq = spec.find('=');
      if (eq != std::string::npos) s.bless[spec.substr(0, eq)] = spec.substr(eq + 1);
    }
    else if (a == "--replay-world") s.replayWorldPath = next("");
    else if (a == "--p99-budget-ms") s.p99BudgetMs = std::atof(next("10").c_str());
    else if (a == "--no-register") s.allowRegister = false;  // T-109 gate
    else {
      std::fprintf(stderr,
                   "usage: bh_server [--map M] [--db D] [--port P] [--soak-secs S] "
                   "[--p99-budget-ms N] [--no-register]\n");
      return 2;
    }
  }

  std::string err;
  if (!s.world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    return 1;
  }
  // T-036/T-035: load every zone that's present (optional on old deployments)
  for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                   {3, "assets/maps/thornwall_crypt.bhmap"},
                                   {4, "assets/maps/bonehowl_mine.bhmap"},
                                   {5, "assets/maps/drowned_crypt.bhmap"}}) {
    std::string zerr;
    if (s.world.loadZone(zid, zpath, &zerr)) {
      const sim::Map* zm = s.world.zoneMap(zid);
      std::printf("[boot] zone %u (%s: %dx%d) online\n", static_cast<unsigned>(zid),
                  zpath, zm ? zm->w : 0, zm ? zm->h : 0);
      std::fflush(stdout);
    } else {
      std::printf("[boot] zone %u (%s) unavailable: %s — its portals will refuse\n",
                  static_cast<unsigned>(zid), zpath, zerr.c_str());
    }
  }
  if (!s.db.open(dbPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    return 1;
  }
  // T-109: make the auth posture visible in every server log.
  std::printf("[auth] registration %s; limiter: %d logins + %d new accounts "
              "per %llds per IP, %d consecutive bad-password fails -> %llds lockout\n",
              s.allowRegister ? "OPEN (prototype posture; --no-register to gate)"
                              : "GATED (--no-register)",
              LoginLimiter::kMaxLoginsPerWindow,
              LoginLimiter::kMaxRegistersPerWindow,
              static_cast<long long>(LoginLimiter::kWindowMs / 1000),
              LoginLimiter::kMaxConsecutiveFails,
              static_cast<long long>(LoginLimiter::kLockoutMs / 1000));
  std::fflush(stdout);
  if (!s.replayWorldPath.empty()) {
    return runReplayWorld(s.replayWorldPath, mapPath);  // offline deterministic mode
  }
  if (!s.recordWorldPath.empty()) {
    s.journal = std::fopen(s.recordWorldPath.c_str(), "w");
    if (s.journal == nullptr) {
      std::fprintf(stderr, "bh_server: cannot open journal '%s'\n",
                   s.recordWorldPath.c_str());
      return 1;
    }
    // T-049x: line-buffer the journal. The repro harness SIGTERMs the server
    // mid-soak (bots finish first); a block-buffered journal lost its
    // unflushed tail on kill — run3p died with a HALF-WRITTEN hash line
    // ("h 875 a52a7e3"), and the replay dutifully failed on the garbage
    // expected hash. Complete lines must survive an ungraceful kill.
    std::setvbuf(s.journal, nullptr, _IOLBF, 0);
    std::fprintf(s.journal, "v %d\n", kJournalEpoch);
    std::fflush(s.journal);
    std::fprintf(stderr, "[journal] recording world to %s (epoch %d)\n",
                 s.recordWorldPath.c_str(), kJournalEpoch);
  }
  if (enet_initialize() != 0) {
    std::fprintf(stderr, "bh_server: enet_initialize failed\n");
    return 1;
  }
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = port;
  s.host = enet_host_create(&addr, 1024, 2, 0, 0);
  if (s.host == nullptr) {
    std::fprintf(stderr, "bh_server: enet_host_create failed\n");
    return 1;
  }
  std::printf("[boot] bh_server on 0.0.0.0:%u | map '%s' | db '%s' | tick 20Hz | aoi %d\n",
              port, mapPath.c_str(), dbPath.c_str(), kAoiRadius);
  std::fflush(stdout);

  s.startEpochSec = ::time(nullptr);
  auto nextTick = std::chrono::steady_clock::now();
  auto startTime = nextTick;

  for (;;) {
    // pump network with a small budget so ticks stay on schedule
    const auto now = std::chrono::steady_clock::now();
    auto msLeft = std::chrono::duration_cast<std::chrono::milliseconds>(nextTick - now).count();
    int svcMs = msLeft > 2 ? 2 : (msLeft > 0 ? static_cast<int>(msLeft) : 0);
    ENetEvent ev;
    while (enet_host_service(s.host, &ev, svcMs) > 0) {
      svcMs = 0;
      switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
          s.sessions.emplace(ev.peer, Session{});
          s.sessions[ev.peer].peer = ev.peer;
          break;
        case ENET_EVENT_TYPE_RECEIVE: {
          ++s.packetsIn;
          Session& sess = s.sessions[ev.peer];
          const auto pv = proto::view(static_cast<std::uint8_t*>(ev.packet->data),
                                      ev.packet->dataLength);
          if (pv.ok) {
            // T-104 boundary guard: a hostile payload must never be able to
            // escape handlePacket as an exception (uncaught => std::terminate
            // => whole world down). Log, drop the offender, keep serving.
            try {
              handlePacket(s, sess, pv);
            } catch (const std::exception& ex) {
              std::fprintf(stderr,
                           "[net] packet exception (%s) — dropping session\n",
                           ex.what());
              std::fflush(stderr);
              dropSession(s, sess);  // erases from s.sessions; sess is dead here
            } catch (...) {
              std::fprintf(stderr,
                           "[net] unknown packet exception — dropping session\n");
              std::fflush(stderr);
              dropSession(s, sess);
            }
          }
          enet_packet_destroy(ev.packet);
          break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
          s.loginLimiter.prune(steadyNowMs());  // T-109: keep the IP map live-only
          auto it = s.sessions.find(ev.peer);
          if (it != s.sessions.end()) {
            journalDisconnect(s, it->second);
            dropSession(s, it->second);
          }
          break;
        }
        default:
          break;
      }
    }

    const auto now2 = std::chrono::steady_clock::now();
    if (now2 >= nextTick) {
      tickServer(s);
      nextTick += std::chrono::microseconds(kTickMicros);
      if (nextTick < now2 - std::chrono::milliseconds(250)) {
        std::fprintf(stderr, "[warn] tick budget blown; resynchronizing\n");
        nextTick = now2 + std::chrono::microseconds(kTickMicros);
      }
    }

    if (s.soakSecs > 0 &&
        now2 - startTime >= std::chrono::seconds(s.soakSecs)) {
      break;
    }
  }

  // ---- soak verdict (M1 gate) ----
  std::vector<std::int64_t> sorted = s.tickMicros;
  std::sort(sorted.begin(), sorted.end());
  const std::int64_t p50 = sorted.empty() ? 0 : sorted[sorted.size() / 2];
  const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
  const double p99ms = static_cast<double>(p99) / 1000.0;
  std::printf("[soak] FINISH ticks=%lld online=%zu entities=%zu tickUs p50=%lld p99=%lld\n",
              static_cast<long long>(s.tick), s.sessions.size(), s.world.count(),
              static_cast<long long>(p50), static_cast<long long>(p99));
  const bool ok = p99ms <= s.p99BudgetMs;
  std::printf("[soak] %s p99=%.2fms budget=%.2fms\n", ok ? "OK" : "FAIL", p99ms, s.p99BudgetMs);
  // balancer report (T-031): one line per mob kind, sorted by level
  for (const content::MobDef& d : content::kMobs) {
    const auto it = s.world.killStats.find(d.mobId);
    if (it == s.world.killStats.end() || it->second.kills == 0) continue;
    const double ttkSec = it->second.emaTtkTicks * 0.2;
    std::printf("[balance] %-16s L%-2d kills=%-5lld TTK(25t hps)=%.1fs xp=%u\n",
                d.name, d.level, static_cast<long long>(it->second.kills),
                ttkSec, d.xp);
  }
  std::fflush(stdout);
  std::fflush(stdout);

  for (auto& kv : s.sessions) {
    if (kv.second.inWorld) journalDisconnect(s, kv.second);  // deterministic close
    enet_peer_disconnect_now(kv.first, 0);
  }
  if (s.journal != nullptr) {
    journalTickHash(s);  // final marker: the wipe/final state
    std::fclose(s.journal);
    s.journal = nullptr;
  }
  enet_host_destroy(s.host);
  enet_deinitialize();
  return ok ? 0 : 2;
}

}  // namespace
}  // namespace bh::server

int main(int argc, char** argv) { return bh::server::run(argc, argv); }
