// bh_server - BLOODHOLLOW headless authoritative world server (Phase 1-2).
// One process, one zone (Thornwall); zones-in-process per ADR-003.
// 20 Hz fixed tick. Protocol v0 (shared/protocol/messages.md).
// Sprint 5 (P2): stats/XP/levels, melee resolve, mobs from map spawners.
#include <algorithm>
#include <cctype>
#include <map>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <enet/enet.h>

#include "persist.h"
#include "loginlimit.h"
#include "protocol/messages_gen.h"
#include "content/wirekind.h"
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
  // T-136 rehearsal posture: opens the siege window unconditionally (loud,
  // never default). Journal-guarded: cross-mode replays refuse (exit 4).
  bool siegeRehearsal = false;
  FILE* journal = nullptr;  std::unordered_map<ENetPeer*, std::uint32_t> loginIndexPerPeer{};
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
  // T-152 GM authority
  std::unordered_set<std::string> gmAllow{};  // lowercased names from BH_GM_NAMES + gm_accounts
  std::string gmLogPath = "logs/gm.log";
  std::string banLogPath = "logs/bans.log";
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
void pushSiegeState(Server& s, Session& sess);
void pushPledgeRoster(Server& s, Session& sess);

// ---- T-152 GM helpers ----------------------------------------------------
inline std::string gmToLower(const std::string& in) {
  std::string r = in;
  std::transform(r.begin(), r.end(), r.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return r;
}
inline bool gmIsOperator(const Server& s, const std::string& name) {
  return s.gmAllow.find(gmToLower(name)) != s.gmAllow.end();
}
inline void gmAppendLog(const std::string& path, const std::string& line) {
  std::error_code ec;
  auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent, ec);
  if (FILE* f = std::fopen(path.c_str(), "a")) {
    std::fputs(line.c_str(), f);
    std::fclose(f);
  }
}
inline void gmLoadAllowlist(Server& s) {
  std::unordered_set<std::string> allow;
  if (const char* env = std::getenv("BH_GM_NAMES")) {
    std::string v(env);
    size_t pos = 0;
    while (pos < v.size()) {
      size_t comma = v.find(',', pos);
      std::string tok = v.substr(pos, comma == std::string::npos ? std::string::npos
                                                                  : comma - pos);
      // trim
      size_t a = tok.find_first_not_of(" \t\r\n");
      size_t b = tok.find_last_not_of(" \t\r\n");
      if (a != std::string::npos && b != std::string::npos)
        tok = tok.substr(a, b - a + 1);
      else
        tok.clear();
      if (!tok.empty()) allow.insert(gmToLower(tok));
      if (comma == std::string::npos) break;
      pos = comma + 1;
    }
  }
  std::vector<std::string> fromDb;
  std::string err;
  if (s.db.loadGmAccounts(&fromDb, &err)) {
    for (auto& n : fromDb) allow.insert(gmToLower(n));
  }
  s.gmAllow = std::move(allow);
  std::printf("[gm] allowlist %zu entries", s.gmAllow.size());
  if (!s.gmAllow.empty()) {
    std::printf(":");
    for (auto& n : s.gmAllow) std::printf(" %s", n.c_str());
  }
  std::printf("\n");
  std::fflush(stdout);
}

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
// persistence: 21; T-126 affix roll widens 1..3 -> 1..10 (shifts every
// downstream draw in journals containing gear drops): 22; T-127 Old Maw
// unique rows draw per-row range(1,100) on 1012 kills: 23; T-128 trio rows
// (1013/1014/1009) extend the same draws: 24; T-135 Weeping Castle boots
// as zone 6 (spawners + gates + heartstone shift the entity set): 25;
// T-138 rebase of T-122: pledge registrar post (world composition, T-112
// precedent) + pledge kinds 34-40 + g-sidecar + schema v13: 26; merge
// repair (2026-09-16): the H1/H2 mine+steward lane (ore nodes map 4,
// Castle Steward map 6) merged under epoch-26 journals shifts the entity
// set (T-068 precedent) — t138/t140 re-replay 12-14/14 mismatches: 27.
// Replay refuses non-matching epoch journals instead of lying with them.
// T-151: siege+pledge wire + HUD (messages 117..119, no sim change but wire
// incompatible — kProtocolVersion 237→241): epoch 28, fresh leg epoch28.bwj.
// T-159: loot depth (rarity roll + 5 slots + affixes 11..20 hooks + ItemSlot
// rarity wire 241→242): sim + RNG-stream change → epoch 29.
constexpr int kJournalEpoch = 29;  // T-159 loot depth. Fresh gate leg: logs/t159.bwj (8 fighters x30s, replay mm=0)

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

// T-122: write the live pledge registry through to SQLite. Upserts current
// rows, deletes ids that vanished (disband) since we last saw them.
void flushPledges(Server& s) {
  static std::set<std::uint32_t> known;  // ids seen this boot
  std::set<std::uint32_t> live;
  for (const auto& p : s.world.pledges()) {
    live.insert(p.id);
    PledgeRec r;
    r.id = p.id;
    r.name = p.name;
    r.emblem = p.emblem;
    r.liege = p.liege;
    r.vault = p.vault;  // T-140 tax-only pool
    s.db.upsertPledge(r, nullptr);
    known.insert(p.id);
  }
  for (std::uint32_t id : known)
    if (live.count(id) == 0) s.db.deletePledge(id, nullptr);
  s.world.pledgesDirty = false;
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
  // T-130: town/EK sidecar (same precedent — missing line replays as 0,0).
  std::fprintf(s.journal, "w %lld %u %u %u\n",
               static_cast<long long>(s.tick + 1), idx,
               static_cast<unsigned>(row.townId), static_cast<unsigned>(row.ek));
  // T-122/T-138: pledge membership sidecar (k-line pattern). Pre-v13
  // journals have no g-lines and parse/replay unchanged. Uses the row
  // (the entity's fields are restored right AFTER journalLogin).
  std::fprintf(s.journal, "g %lld %u %u %u\n",
               static_cast<long long>(s.tick + 1), idx,
               static_cast<unsigned>(row.pledgeId > 0 ? row.pledgeId : 0),
               static_cast<unsigned>(row.pledgeRank));
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
                          static_cast<std::int64_t>(e->swingLands),
                          static_cast<int>(e->townId),
                          static_cast<int>(e->ek),
                          static_cast<int>(e->pledgeId),
                          static_cast<int>(e->pledgeRank));
      }
      // T-122: mirror registry changes (create/invite/leave/kick/disband)
      // into SQLite. Posture matches gold: unsaved changes die with a kill.
      if (s.world.pledgesDirty) flushPledges(s);
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
      // (1=bad credentials, 2=invalid name, 3=db error, 4=protocol mismatch, 7=banned.)
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
      // T-152: banned users refused before any account creation.
      {
        bool banned = false;
        std::string banReason;
        std::int64_t banExp = 0;
        std::string banErr;
        if (s.db.isBanned(h.username, &banned, &banReason, &banExp, &banErr) && banned) {
          const std::int64_t nowSec = ::time(nullptr);
          if (banExp == 0 || banExp > nowSec) {
            LoginResult r;
            r.ok = 0;
            r.reason = 7;  // banned
            sendMsg(sess.peer, r, s);
            KickNotice kn;
            kn.reason = std::string("banned") +
                        (banReason.empty() ? "" : std::string(": ") + banReason) +
                        (banExp == 0 ? " (permanent)"
                                     : " (until " + std::to_string(banExp) + ")");
            sendMsg(sess.peer, kn, s);
            std::printf("[ban] login denied banned=%s by_exp=%lld reason='%s'\n",
                        h.username.c_str(), static_cast<long long>(banExp),
                        banReason.c_str());
            std::fflush(stdout);
            gmAppendLog(s.banLogPath,
                        "[ban-denied] user=" + h.username +
                            " expires=" + std::to_string(banExp) +
                            " reason='" + banReason + "'\n");
            gmAppendLog(s.gmLogPath,
                        "[ban-denied] user=" + h.username +
                            " expires=" + std::to_string(banExp) +
                            " reason='" + banReason + "'\n");
            return;
          } else {
            std::string delErr;
            s.db.deleteBan(h.username, &delErr);
          }
        }
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
        // T-130: town war identity + EK fame (schema v12; clamped, era-safe)
        pe->townId = static_cast<std::uint8_t>(row.townId < 0 ? 0 : (row.townId > 2 ? 0 : row.townId));
        pe->ek = static_cast<std::uint32_t>(row.ek < 0 ? 0 : row.ek);
        // T-122: pledge membership (schema v12; defaults 0 for old rows).
        // Registry itself was loaded at boot; this restores the cache.
        pe->pledgeId = static_cast<std::uint32_t>(row.pledgeId < 0 ? 0 : row.pledgeId);
        pe->pledgeRank = static_cast<std::uint8_t>(row.pledgeRank);
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
      pushSiegeState(s, sess);
      pushPledgeRoster(s, sess);
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
      // T-152 GM authority gate
      auto gmDenyTmp = [&](const std::string& verb) {
        proto::ChatMsg dm;
        dm.channel = 2;
        dm.from = "";
        dm.text = "gm denied: operator only (" + verb + ")";
        sendMsg(sess.peer, dm, s);
        std::printf("[gm-denied] tick=%lld name=%s verb=%s\n",
                    static_cast<long long>(s.tick), sess.user.c_str(), verb.c_str());
        std::fflush(stdout);
        gmAppendLog(s.gmLogPath,
                    "[gm-denied] tick=" + std::to_string(s.tick) + " name=" +
                        sess.user + " verb=" + verb + "\n");
      };
      auto isGmTmp = [&]() -> bool { return gmIsOperator(s, sess.user); };
      if (m.text == "gm blood-moon") {  // T-129: journaled, replay-exact (H1 shape)
        if (!isGmTmp()) {
          gmDenyTmp("gm blood-moon");
          break;
        }
        std::printf("[gm] %s executes gm blood-moon\n", sess.user.c_str());
        std::fflush(stdout);
        gmAppendLog(s.gmLogPath, "[gm] tick=" + std::to_string(s.tick) +
                                     " by=" + sess.user + " verb=gm blood-moon\n");
        Command c;
        c.kind = Command::kBloodMoon;
        if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
        break;
      }
      if (m.text == "gm siege-start") {  // T-131: in-window + bands, else quiet
        if (!isGmTmp()) {
          gmDenyTmp("gm siege-start");
          break;
        }
        std::printf("[gm] %s executes gm siege-start\n", sess.user.c_str());
        std::fflush(stdout);
        gmAppendLog(s.gmLogPath, "[gm] tick=" + std::to_string(s.tick) +
                                     " by=" + sess.user + " verb=gm siege-start\n");
        Command c;
        c.kind = Command::kSiegeStart;
        if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
        break;
      }
      if (m.text == "gm ek") {  // T-130 board readout (directed) — FIXED: was dead inside slash block
        if (!isGmTmp()) {
          gmDenyTmp("gm ek");
          break;
        }
        if (Entity* me = s.world.find(sess.entityId)) s.world.ekReadout(*me);
        gmAppendLog(s.gmLogPath, "[gm] tick=" + std::to_string(s.tick) +
                                     " by=" + sess.user + " verb=gm ek\n");
        break;
      }
      if (m.text == "gm siege") {  // T-134 castle readout (directed) — FIXED: was dead inside slash block
        if (!isGmTmp()) {
          gmDenyTmp("gm siege");
          break;
        }
        if (Entity* me = s.world.find(sess.entityId)) s.world.siegeReadout(*me);
        gmAppendLog(s.gmLogPath, "[gm] tick=" + std::to_string(s.tick) +
                                     " by=" + sess.user + " verb=gm siege\n");
        break;
      }
      if (m.text.rfind("gm announce ", 0) == 0) {  // T-152 announce: ch2 broadcast, journaled as y-line
        if (!isGmTmp()) {
          gmDenyTmp("gm announce");
          break;
        }
        std::string raw = m.text.substr(12);
        std::string ann = sanitizeChat(raw);
        if (ann.empty()) break;
        std::string out = std::string("[ANNOUNCE] ") + ann;
        broadcastChat(s, 2, "", out);
        std::printf("[gm-announce] tick=%lld by=%s text='%s'\n",
                    static_cast<long long>(s.tick), sess.user.c_str(), ann.c_str());
        std::fflush(stdout);
        gmAppendLog(s.gmLogPath, "[gm-announce] tick=" + std::to_string(s.tick) +
                                     " by=" + sess.user + " text='" + ann + "'\n");
        gmAppendLog(s.banLogPath, "[gm-announce] tick=" + std::to_string(s.tick) +
                                      " by=" + sess.user + " text='" + ann + "'\n");
        if (s.journal != nullptr) {
          auto it = s.loginIndexPerPeer.find(sess.peer);
          if (it != s.loginIndexPerPeer.end()) {
            std::fprintf(s.journal, "y %lld %u %s\n", static_cast<long long>(s.tick),
                         it->second, ann.c_str());
            std::fflush(s.journal);
          }
        }
        break;
      }
      if (!m.text.empty() && m.text[0] == '/') {  // party verbs (era commands)
        // T-152 operator bans/kicks (must be before party /kick so GM's verb is session-level)
        if (m.text.rfind("/ban ", 0) == 0) {
          if (!isGmTmp()) {
            gmDenyTmp("/ban");
            break;
          }
          std::string rest = m.text.substr(5);
          size_t p0 = rest.find_first_not_of(" \t");
          if (p0 != std::string::npos) rest = rest.substr(p0);
          else rest.clear();
          if (rest.empty()) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "usage: /ban <name> <minutes> [reason]";
            sendMsg(sess.peer, um, s);
            break;
          }
          size_t sp1 = rest.find_first_of(" \t");
          std::string tname;
          std::string after;
          if (sp1 == std::string::npos) {
            tname = rest;
            after = "";
          } else {
            tname = rest.substr(0, sp1);
            after = rest.substr(sp1);
            size_t q = after.find_first_not_of(" \t");
            if (q != std::string::npos) after = after.substr(q);
            else after.clear();
          }
          bool nameOk = tname.size() >= 3 && tname.size() <= 16;
          for (char c : tname) {
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-';
            if (!ok) nameOk = false;
          }
          if (!nameOk) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "invalid name for /ban";
            sendMsg(sess.peer, um, s);
            break;
          }
          if (after.empty()) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "usage: /ban <name> <minutes> [reason]";
            sendMsg(sess.peer, um, s);
            break;
          }
          size_t sp2 = after.find_first_of(" \t");
          std::string minsStr;
          std::string reason;
          if (sp2 == std::string::npos) {
            minsStr = after;
            reason = "";
          } else {
            minsStr = after.substr(0, sp2);
            reason = after.substr(sp2);
            size_t r = reason.find_first_not_of(" \t");
            if (r != std::string::npos) reason = reason.substr(r);
            else reason.clear();
            if (reason.size() > 120) reason = reason.substr(0, 120);
            std::string cleaned;
            for (unsigned char c : reason)
              if (c >= 32 && c <= 126) cleaned.push_back((char)c);
            reason = cleaned;
          }
          bool minsOk = !minsStr.empty() && minsStr.size() <= 7;
          std::int64_t minutes = 0;
          for (char c : minsStr)
            if (c < '0' || c > '9') minsOk = false;
          if (minsOk) {
            for (char c : minsStr) minutes = minutes * 10 + (c - '0');
            if (minutes <= 0 || minutes > 5256000) minsOk = false;
          }
          if (!minsOk) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "invalid minutes (1..5256000)";
            sendMsg(sess.peer, um, s);
            break;
          }
          std::int64_t nowSec = ::time(nullptr);
          std::int64_t expires = nowSec + minutes * 60;
          std::string dbErr;
          if (!s.db.upsertBan(tname, expires, reason, sess.user, &dbErr)) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "ban failed: " + dbErr;
            sendMsg(sess.peer, um, s);
            break;
          }
          std::printf(
              "[ban] tick=%lld by=%s target=%s minutes=%lld reason='%s' "
              "expires=%lld\n",
              static_cast<long long>(s.tick), sess.user.c_str(), tname.c_str(),
              static_cast<long long>(minutes), reason.c_str(),
              static_cast<long long>(expires));
          std::fflush(stdout);
          gmAppendLog(s.banLogPath,
                      "[ban] tick=" + std::to_string(s.tick) + " by=" +
                          sess.user + " target=" + tname +
                          " minutes=" + std::to_string(minutes) + " reason='" +
                          reason + "' expires=" + std::to_string(expires) + "\n");
          gmAppendLog(s.gmLogPath,
                      "[ban] tick=" + std::to_string(s.tick) + " by=" +
                          sess.user + " target=" + tname +
                          " minutes=" + std::to_string(minutes) + " reason='" +
                          reason + "' expires=" + std::to_string(expires) + "\n");
          ENetPeer* targetPeer = nullptr;
          for (auto& kv : s.sessions) {
            if (gmToLower(kv.second.user) == gmToLower(tname)) {
              targetPeer = kv.first;
              break;
            }
          }
          if (targetPeer != nullptr) {
            proto::KickNotice kn;
            kn.reason = "banned for " + std::to_string(minutes) + "m" +
                        (reason.empty() ? "" : std::string(": ") + reason);
            sendMsg(targetPeer, kn, s);
            broadcastChat(
                s, 2, "",
                tname + " was banned for " + std::to_string(minutes) + "m by " +
                    sess.user + (reason.empty() ? "" : " (" + reason + ")"));
            enet_peer_disconnect_later(targetPeer, 0);
          } else {
            broadcastChat(
                s, 2, "",
                tname + " was banned for " + std::to_string(minutes) + "m by " +
                    sess.user + (reason.empty() ? "" : " (" + reason + ")"));
          }
          {
            proto::ChatMsg cm;
            cm.channel = 2;
            cm.from = "";
            cm.text = "banned " + tname + " for " + std::to_string(minutes) + "m.";
            sendMsg(sess.peer, cm, s);
          }
          break;
        }
        if (m.text.rfind("/unban ", 0) == 0) {
          if (!isGmTmp()) {
            gmDenyTmp("/unban");
            break;
          }
          std::string rest = m.text.substr(7);
          size_t p0 = rest.find_first_not_of(" \t");
          if (p0 != std::string::npos) rest = rest.substr(p0);
          else rest.clear();
          size_t sp = rest.find_first_of(" \t");
          std::string tname = sp == std::string::npos ? rest : rest.substr(0, sp);
          size_t a = tname.find_first_not_of(" \t\r\n");
          size_t b = tname.find_last_not_of(" \t\r\n");
          if (a != std::string::npos && b != std::string::npos)
            tname = tname.substr(a, b - a + 1);
          else
            tname.clear();
          bool nameOk = tname.size() >= 3 && tname.size() <= 16;
          for (char c : tname) {
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-';
            if (!ok) nameOk = false;
          }
          if (!nameOk) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "invalid name for /unban";
            sendMsg(sess.peer, um, s);
            break;
          }
          std::string dbErr;
          if (!s.db.deleteBan(tname, &dbErr)) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "unban failed: " + dbErr;
            sendMsg(sess.peer, um, s);
            break;
          }
          std::printf("[unban] tick=%lld by=%s target=%s\n",
                      static_cast<long long>(s.tick), sess.user.c_str(),
                      tname.c_str());
          std::fflush(stdout);
          gmAppendLog(s.banLogPath, "[unban] tick=" + std::to_string(s.tick) +
                                        " by=" + sess.user + " target=" + tname + "\n");
          gmAppendLog(s.gmLogPath, "[unban] tick=" + std::to_string(s.tick) +
                                       " by=" + sess.user + " target=" + tname + "\n");
          broadcastChat(s, 2, "", tname + " was unbanned by " + sess.user);
          {
            proto::ChatMsg cm;
            cm.channel = 2;
            cm.from = "";
            cm.text = "unbanned " + tname;
            sendMsg(sess.peer, cm, s);
          }
          break;
        }
        if (m.text.rfind("/kick ", 0) == 0 && isGmTmp()) {
          std::string rest = m.text.substr(6);
          size_t p0 = rest.find_first_not_of(" \t");
          if (p0 != std::string::npos) rest = rest.substr(p0);
          else rest.clear();
          size_t sp = rest.find_first_of(" \t");
          std::string tname = sp == std::string::npos ? rest : rest.substr(0, sp);
          size_t a = tname.find_first_not_of(" \t\r\n");
          size_t b = tname.find_last_not_of(" \t\r\n");
          if (a != std::string::npos && b != std::string::npos)
            tname = tname.substr(a, b - a + 1);
          else
            tname.clear();
          bool nameOk = tname.size() >= 3 && tname.size() <= 16;
          for (char c : tname) {
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-';
            if (!ok) nameOk = false;
          }
          if (!nameOk) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = "invalid name for /kick";
            sendMsg(sess.peer, um, s);
            break;
          }
          ENetPeer* targetPeer = nullptr;
          std::string actualName;
          for (auto& kv : s.sessions) {
            if (gmToLower(kv.second.user) == gmToLower(tname)) {
              targetPeer = kv.first;
              actualName = kv.second.user;
              break;
            }
          }
          if (targetPeer == nullptr) {
            proto::ChatMsg um;
            um.channel = 2;
            um.from = "";
            um.text = tname + " is not online";
            sendMsg(sess.peer, um, s);
            break;
          }
          std::printf("[kick] tick=%lld by=%s target=%s\n",
                      static_cast<long long>(s.tick), sess.user.c_str(),
                      actualName.c_str());
          std::fflush(stdout);
          gmAppendLog(s.banLogPath, "[kick] tick=" + std::to_string(s.tick) +
                                        " by=" + sess.user + " target=" + actualName + "\n");
          gmAppendLog(s.gmLogPath, "[kick] tick=" + std::to_string(s.tick) +
                                       " by=" + sess.user + " target=" + actualName + "\n");
          proto::KickNotice kn;
          kn.reason = "kicked by operator " + sess.user;
          sendMsg(targetPeer, kn, s);
          broadcastChat(s, 2, "", actualName + " was kicked by " + sess.user);
          enet_peer_disconnect_later(targetPeer, 0);
          {
            proto::ChatMsg cm;
            cm.channel = 2;
            cm.from = "";
            cm.text = "kicked " + actualName;
            sendMsg(sess.peer, cm, s);
          }
          break;
        }
        // T-122 pledge chat + roster: chat-class (no sim effect, never
        // journaled — replay regenerates nothing it needs).
        if (m.text.rfind("/p ", 0) == 0) {
          Entity* pe = s.world.find(sess.entityId);
          if (pe != nullptr) s.world.pledgeChat(*pe, m.text.substr(3));
          break;
        }
        if (m.text == "/pledge who") {
          Entity* pe = s.world.find(sess.entityId);
          if (pe != nullptr) s.world.pledgeWho(*pe);
          break;
        }
        if (m.text == "/pledge vault") {  // T-140 readout (directed, unjournaled)
          Entity* pe = s.world.find(sess.entityId);
          if (pe != nullptr) s.world.pledgeVaultReadout(*pe);
          break;
        }
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
        else if (m.text == "/mine") { c.kind = Command::kMine; }
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
        }
        else if (m.text.rfind("/oath ", 0) == 0) {  // T-130 town swear (L19+)
          const std::string k = m.text.substr(6);
          std::uint8_t town = 0;
          if (k == "thornwall" || k == "ashen") town = 1;
          else if (k == "marrowgate" || k == "synod") town = 2;
          if (town != 0) {
            c.kind = Command::kOath;
            c.a = town;
          } else {
            okCmd = false;  // unknown town: say it aloud, era-right
          }
        }
        else if (m.text == "/siege-reg") {  // T-131: speaker captains a band
          c.kind = Command::kSiegeReg;
        }
        else if (m.text == "/breach") {  // T-132: ram work on a near gate
          c.kind = Command::kBreach;
        }
        else if (m.text == "/crown") {  // T-133: kneel at the attuned stone
          c.kind = Command::kCrown;
        } else if (m.text.rfind("/pledge ", 0) == 0) {  // T-122 pledge-lite
          const std::string arg = m.text.substr(8);
          // name->id resolution happens here, pre-journal (kDuel pattern):
          // the c-line carries ints only.
          auto resolve = [&](const std::string& nm) -> bool {
            for (const auto& e : s.world.entities()) {
              if (e.kind == EntityKind::kPlayer && e.name == nm) {
                c.a = static_cast<std::int32_t>(e.id);
                return true;
              }
            }
            return false;
          };
          if (arg.rfind("create ", 0) == 0) {
            const std::string nm = arg.substr(7);
            // shape check here so typos fall through as an ordinary say;
            // the World method re-checks everything (replay safety)
            const int n = static_cast<int>(nm.size());
            bool shaped = n >= World::kPledgeNameMin && n <= World::kPledgeNameMax;
            for (int i = 0; shaped && i < n; ++i) {
              const char ch = nm[i];
              shaped = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                       (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
            }
            if (shaped) {
              c.kind = Command::kPledgeCreate;
              c.text = nm;  // live-only: the c-line journal drops it
            } else {
              okCmd = false;
            }
          } else if (arg.rfind("invite ", 0) == 0) {
            c.kind = Command::kPledgeInvite;
            okCmd = resolve(arg.substr(7));
          } else if (arg == "accept") {
            c.kind = Command::kPledgeAccept;
          } else if (arg == "leave") {
            c.kind = Command::kPledgeLeave;
          } else if (arg.rfind("kick ", 0) == 0) {
            c.kind = Command::kPledgeKick;
            okCmd = resolve(arg.substr(5));
          } else if (arg.rfind("promote ", 0) == 0) {
            c.kind = Command::kPledgeRank;
            c.channel = 2;  // Bloodsworn
            okCmd = resolve(arg.substr(8));
          } else if (arg.rfind("demote ", 0) == 0) {
            c.kind = Command::kPledgeRank;
            c.channel = 1;  // Initiate
            okCmd = resolve(arg.substr(7));
          } else if (arg == "disband") {
            c.kind = Command::kPledgeDisband;
          } else if (arg.rfind("tithe ", 0) == 0) {  // T-140 voluntary tithe
            // digits-only, capped: typos fall through as an ordinary say.
            const std::string digits = arg.substr(6);
            std::int32_t amount = 0;
            bool shaped = !digits.empty() && digits.size() <= 7;
            for (char ch : digits) {
              if (!shaped || ch < '0' || ch > '9') { shaped = false; break; }
              amount = amount * 10 + (ch - '0');
            }
            if (shaped && amount > 0) {
              c.kind = Command::kPledgeTithe;
              c.a = amount;
            } else {
              okCmd = false;
            }
          } else {
            okCmd = false;
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
    m.rarity = e->inv[i].rarity;          // T-159
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

// T-151: siege + pledge HUD (wire 117..119, parchment era)
void pushSiegeState(Server& s, Session& sess) {
  // holder pledge lookup (pledge that contains the holder's name)
  std::uint32_t hpId = 0;
  std::string hpName;
  const std::string& holder = s.world.siegeHolderName();
  if (!holder.empty()) {
    for (const auto& p : s.world.pledges()) {
      for (const auto& m : p.members) if (m == holder) { hpId = p.id; hpName = p.name; break; }
      if (hpId != 0) break;
    }
  }
  std::uint16_t gh0 = 0, gh1 = 0;
  std::vector<std::uint32_t> gh;
  for (const auto& e : s.world.entities()) if (e.wireKind == content::kWireKindSiegeGate) gh.push_back(e.hp);
  std::sort(gh.begin(), gh.end()); // stable by hp isn't id-stable, but gate count ≤2 so sort by hp is deterministic for display
  // recover ordered by id for client: collect gates sorted by id
  std::vector<const Entity*> gates;
  for (const auto& e : s.world.entities()) if (e.wireKind == content::kWireKindSiegeGate) gates.push_back(&e);
  std::sort(gates.begin(), gates.end(), [](const Entity* a, const Entity* b){ return a->id < b->id; });
  if (gates.size() > 0) gh0 = static_cast<std::uint16_t>(gates[0]->hp);
  if (gates.size() > 1) gh1 = static_cast<std::uint16_t>(gates[1]->hp);
  (void)gh;
  std::uint32_t cOwner = 0; std::string cName; std::uint32_t cDead = 0;
  for (const auto& e : s.world.entities()) {
    if (e.kind != EntityKind::kPlayer || e.dead) continue;
    if (e.crownUntil >= 0 && s.world.tickCount() < e.crownUntil) { cOwner = e.id; cName = e.name; cDead = static_cast<std::uint32_t>(e.crownUntil); break; }
  }
  std::uint8_t phase = 0;
  if (cOwner != 0) phase = 4;
  else if (s.world.heartAttuned()) phase = 3;
  else if (s.world.siegeBattleActive()) phase = 2;
  else if (s.world.inSiegeWindow()) phase = 1;
  else phase = 0;
  proto::SiegeState st;
  st.holderPledgeId = hpId;
  st.holderPledgeName = hpName;
  st.holderName = holder;
  st.windowEndTick = static_cast<std::uint32_t>(s.world.siegeWindowEnd());
  st.battleEndTick = s.world.siegeBattleActive() ? static_cast<std::uint32_t>(s.world.siegeBattleEndsAt()) : 0;
  st.gateHp0 = gh0; st.gateHp1 = gh1;
  st.heartProgress = static_cast<std::uint16_t>(s.world.heartProgress());
  st.heartAttuned = s.world.heartAttuned() ? 1 : 0;
  st.crownOwnerId = cOwner; st.crownOwnerName = cName; st.crownDeadline = cDead;
  st.bandCount = static_cast<std::uint8_t>(s.world.siegeBandsUsed());
  st.phase = phase;
  st.vaultGold = s.world.siegeVault(); st.crowns = s.world.siegeCrowns();
  sendMsg(sess.peer, st, s);
}
void pushPledgeRoster(Server& s, Session& sess) {
  Entity* me = s.world.find(sess.entityId);
  const World::Pledge* p = nullptr;
  if (me != nullptr && me->pledgeId != 0) p = s.world.pledgeById(me->pledgeId);
  proto::PledgeRoster hdr;
  if (p != nullptr) { hdr.pledgeId = p->id; hdr.name = p->name; hdr.emblem = p->emblem; hdr.count = static_cast<std::uint8_t>(p->members.size()); hdr.vaultGold = p->vault; }
  else { hdr.pledgeId = 0; hdr.name = ""; hdr.emblem = 0; hdr.count = 0; hdr.vaultGold = 0; }
  sendMsg(sess.peer, hdr, s);
  if (p == nullptr) return;
  for (const std::string& mname : p->members) {
    const Entity* found = nullptr;
    for (const auto& e : s.world.entities()) if (e.kind == EntityKind::kPlayer && e.name == mname) { found = &e; break; }
    proto::PledgeMember pm; pm.name = mname;
    if (found != nullptr && found->pledgeId == p->id) { pm.rank = found->pledgeRank; pm.level = found->level; pm.online = 1; }
    else { pm.rank = (mname == p->liege ? 3 : 1); pm.level = 1; pm.online = 0; }
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
          // T-142: players ride class+sex so the client sheets them.
          // Mobs carry none (0 = hero fallback). Sex is 0 (unknown) until
          // T-142b captures it at creation (no source of truth exists yet).
          m.classId = e->kind == EntityKind::kPlayer ? e->classId : std::uint8_t(0);
          m.sex = std::uint8_t(0);
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

  // T-134: throttled castle-memory write (vault accrues per taxed kill).
  if (s.world.siegeSaveDue(s.tick)) {
    std::string siegeErr;
    if (s.db.saveSiege(s.world.siegeHolder(), s.world.siegeHolderName(),
                       s.world.siegeVault(), s.world.siegeCrowns(),
                       &siegeErr)) {
      s.world.markSiegeSaved(s.tick);
    } else {
      std::fprintf(stderr, "bh_server: siege save: %s\n", siegeErr.c_str());
    }
  }

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
        // T-142: players ride class+sex (mobs 0; sex 0 unknown until T-142b).
        m.classId = e->kind == EntityKind::kPlayer ? e->classId : std::uint8_t(0);
        m.sex = std::uint8_t(0);
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
      // T-142: class rides the delta so a /kit oath re-sheets remotes live.
      d.classId = e->kind == EntityKind::kPlayer ? e->classId : std::uint8_t(0);
      d.sex = std::uint8_t(0);
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
    // T-151: siege + pledge HUD every second (wire 117..119)
    for (auto& kv : s.sessions) if (kv.second.inWorld) { pushSiegeState(s, kv.second); pushPledgeRoster(s, kv.second); }
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
int runReplayWorld(const std::string& path, const std::string& mapPath,
                   bool rehearsal) {
  {
    FILE* pf = std::fopen(path.c_str(), "r");
    if (pf != nullptr) {
      char vline[32];
      if (std::fgets(vline, sizeof vline, pf) != nullptr && vline[0] == 'v') {
        int ep = 1;
        char mode[16] = "";
        std::sscanf(vline, "v %d %15s", &ep, mode);
        if (ep != kJournalEpoch) {
          std::fprintf(stderr,
              "[replay] journal epoch %d vs build epoch %d — sim semantics "
              "changed since; record a fresh gate leg (old leg retained as "
              "history)\n", ep, kJournalEpoch);
          std::fclose(pf);
          return 4;
        }
        // T-136: rehearsal journals replay only under the flag and vice versa.
        const bool journalRehearsal = std::string(mode) == "rehearsal";
        if (journalRehearsal != rehearsal) {
          std::fprintf(stderr,
              "[replay] rehearsal-mode mismatch (journal %s rehearsal, "
              "--siege-rehearsal %s) — refusing to lie with it\n",
              journalRehearsal ? "is" : "is not",
              rehearsal ? "set" : "unset");
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
  world.setRehearsal(rehearsal);  // T-136: window opens in rehearsal journals
  if (!world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    std::fclose(f);
    return 1;
  }
  {  // mirror the server's boot set (any zone asset that's actually there)
    for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                     {3, "assets/maps/thornwall_crypt.bhmap"},
                                     {4, "assets/maps/bonehowl_mine.bhmap"},
                                     {5, "assets/maps/drowned_crypt.bhmap"},
                                     {6, "assets/maps/weeping_castle.bhmap"}}) {
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
  std::unordered_map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>>
      loginTowns;  // w-lines: town+ek sidecar (T-130; absent => 0,0)
  std::unordered_map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>>
      loginPledges;  // g-lines (T-122/T-138): idx -> {pledgeId, rank}

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
    } else if (line[0] == 'w') {  // T-130 town/EK sidecar: w tick idx town ek
      long long wtick; unsigned widx, town, ek;
      if (std::sscanf(line, "w %lld %u %u %u", &wtick, &widx, &town, &ek) == 4)
        loginTowns[widx] = {town, ek};
    } else if (line[0] == 'g') {  // T-122/T-138 pledge sidecar: g tick idx pledgeId rank
      long long gtick; unsigned gidx, gpid, grank;
      if (std::sscanf(line, "g %lld %u %u %u", &gtick, &gidx, &gpid, &grank) == 4)
        loginPledges[gidx] = {gpid, grank};
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
    Entity* pe = world.find(e.id);    pe->level = static_cast<std::uint8_t>(L.level < 1 ? 1 : (L.level > 25 ? 25 : L.level));
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
    // T-130: town/EK sidecar (absent in pre-T-130 journals => 0,0)
    if (const auto wi = loginTowns.find(L.idx); wi != loginTowns.end()) {
      pe->townId =
          static_cast<std::uint8_t>(wi->second.first > 2 ? 0 : wi->second.first);
      pe->ek = wi->second.second;
    }
    // T-122/T-138: pledge membership from the g-sidecar (absent => pre-v13
    // journal, unsworn). Stubs keep the replay registry consistent.
    if (const auto gi = loginPledges.find(L.idx); gi != loginPledges.end())
      world.pledgeReplayRestore(*pe, gi->second.first,
                                static_cast<std::uint8_t>(gi->second.second));
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
    else if (a == "--siege-rehearsal") s.siegeRehearsal = true;  // T-136
    else if (a == "--p99-budget-ms") s.p99BudgetMs = std::atof(next("10").c_str());
    else if (a == "--no-register") s.allowRegister = false;  // T-109 gate
    else {
      std::fprintf(stderr,
                   "usage: bh_server [--map M] [--db D] [--port P] [--soak-secs S] "
                   "[--p99-budget-ms N] [--no-register] [--siege-rehearsal]\n");
      return 2;
    }
  }

  if (!s.bless.empty() && (!s.recordWorldPath.empty() || !s.replayWorldPath.empty())) {
    std::fprintf(stderr,
                 "[fatal] --bless is incompatible with --record-world/--replay-world "
                 "(breaks replay determinism)\n");
    return 1;
  }

  std::string err;
  if (!s.world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    return 1;
  }
  // T-036/T-035: load every zone that's present (optional on old deployments)
  // T-135: Weeping Castle (zone 6) joins the boot set.
  for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                   {3, "assets/maps/thornwall_crypt.bhmap"},
                                   {4, "assets/maps/bonehowl_mine.bhmap"},
                                   {5, "assets/maps/drowned_crypt.bhmap"},
                                   {6, "assets/maps/weeping_castle.bhmap"}}) {
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
  // T-152 GM authority + ban prune (out-of-band, epoch-neutral)
  {
    std::string pruneErr;
    s.db.pruneExpiredBans(&pruneErr);
    gmLoadAllowlist(s);
  }
  // T-134: the castle remembers its master across reboots (empty => zeros).
  {
    Db::SiegeRow siege{};
    std::string siegeErr;
    if (s.db.loadSiege(&siege, &siegeErr)) {
      s.world.loadSiegeState(static_cast<std::uint32_t>(siege.holderId),
                             siege.holderName,
                             static_cast<std::uint32_t>(siege.vaultGold),
                             static_cast<std::uint32_t>(siege.crowns));
    } else {
      std::fprintf(stderr, "bh_server: siege load: %s\n", siegeErr.c_str());
    }
  }
  // T-136 rehearsal posture (loud, never default).
  if (s.siegeRehearsal) {
    s.world.setRehearsal(true);
    std::printf("[rehearsal] siege window OPEN unconditionally (drill posture)\n");
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
    return runReplayWorld(s.replayWorldPath, mapPath,
                          s.siegeRehearsal);  // offline deterministic mode
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
    if (s.siegeRehearsal)
      std::fprintf(s.journal, "v %d rehearsal\n", kJournalEpoch);
    else
      std::fprintf(s.journal, "v %d\n", kJournalEpoch);
    std::fflush(s.journal);
    std::fprintf(stderr, "[journal] recording world to %s (epoch %d)\n",
                 s.recordWorldPath.c_str(), kJournalEpoch);
  }
  // T-122: pledge registry — live path only (replay rebuilds from the
  // journal g-sidecar; record mode starts with an empty registry).
  {
    std::vector<PledgeRec> recs;
    std::string perr;
    if (s.db.loadPledges(&recs, &perr)) {
      std::vector<std::pair<std::string, std::pair<int, int>>> mems;
      s.db.loadPledgeMembers(&mems, &perr);
      std::vector<World::Pledge> loaded;
      for (const PledgeRec& r : recs) {
        World::Pledge p;
        p.id = r.id;
        p.name = r.name;
        p.emblem = static_cast<std::uint8_t>(r.emblem);
        p.liege = r.liege;
        p.vault = r.vault;  // T-140 tax-only pool
        loaded.push_back(std::move(p));
      }
      for (const auto& m : mems) {  // (name, {pledgeId, rank})
        for (auto& p : loaded) {
          if (p.id == static_cast<std::uint32_t>(m.second.first)) {
            p.members.push_back(m.first);
            break;
          }
        }
      }
      s.world.setPledges(std::move(loaded));
      std::printf("[boot] pledges: %zu registered, %zu sworn members\n",
                  recs.size(), mems.size());
      std::fflush(stdout);
    } else {
      std::printf("[boot] pledge registry unavailable: %s (continuing)\n",
                  perr.c_str());
    }
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
