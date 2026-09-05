// bh_client - BLOODHOLLOW game client.
// Modes: ONLINE (with --server: real client<->server play) or offline
// free play (default), scripted screenshot demo (--screenshot), journal
// record/replay (--record/--replay) with per-tick hash verification.
// Journal record/replay is explicitly offline-only: the server is
// authoritative in online mode, so local journals would be meaningless.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "core/timestep.h"
#include "game.h"
#include "net_client.h"
#include "sim/bhmap.h"
#include "sim/journal.h"
#include "sim/tick.h"

namespace {
void usage() {
  std::fprintf(stderr,
               "usage: bh_client [--map path.bhmap] [--vsync off]\n"
               "  online:\n"
               "                 [--server HOST]         host:port or IP (enables online)\n"
               "                 [--port N]              default 7777\n"
               "                 [--name USER]           account/character name\n"
               "                 [--pass PASSWORD]\n"
               "  offline:\n"
               "                 [--screenshot PREFIX]   scripted demo + captures\n"
               "  dev:\n"
               "                 [--shot FILE]           screenshot just before --max-ticks exit\n"
               "                 [--record FILE]         journal record (events + hashes)\n"
               "                 [--replay FILE]         journal replay + hash verify\n"
               "                 [--max-ticks N]         exit after N sim ticks\n");
}
constexpr int kFixedTicksPerFrame = 3;  // deterministic pace for scripted modes
}  // namespace

int main(int argc, char** argv) {
  std::string mapPath = "assets/maps/thornwall.bhmap";
  std::string shotPrefix, recordPath, replayPath;
  std::string server, name = "wanderer", pass, shotPath;
  int port = 7777;
  std::int64_t maxTicks = 0;
  bool vsync = true;
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (std::strcmp(a, "--map") == 0 && i + 1 < argc) {
      mapPath = argv[++i];
    } else if (std::strcmp(a, "--vsync") == 0 && i + 1 < argc) {
      vsync = std::strcmp(argv[++i], "off") != 0;
    } else if (std::strcmp(a, "--screenshot") == 0 && i + 1 < argc) {
      shotPrefix = argv[++i];
    } else if (std::strcmp(a, "--record") == 0 && i + 1 < argc) {
      recordPath = argv[++i];
    } else if (std::strcmp(a, "--replay") == 0 && i + 1 < argc) {
      replayPath = argv[++i];
    } else if (std::strcmp(a, "--max-ticks") == 0 && i + 1 < argc) {
      maxTicks = std::atoll(argv[++i]);
    } else if (std::strcmp(a, "--server") == 0 && i + 1 < argc) {
      server = argv[++i];
    } else if (std::strcmp(a, "--port") == 0 && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--shot") == 0 && i + 1 < argc) {
      shotPath = argv[++i];
    } else if (std::strcmp(a, "--name") == 0 && i + 1 < argc) {
      name = argv[++i];
    } else if (std::strcmp(a, "--pass") == 0 && i + 1 < argc) {
      pass = argv[++i];
    } else if (std::strcmp(a, "--help") == 0) {
      usage();
      return 0;
    } else {
      usage();
      return 2;
    }
  }

  const bool online = !server.empty();
  if (online && (!recordPath.empty() || !replayPath.empty() || !shotPrefix.empty())) {
    std::fprintf(stderr,
                 "bh_client: --record/--replay/--screenshot are offline-only modes "
                 "and cannot be combined with --server\n");
    return 2;
  }
  if (!shotPrefix.empty() && !shotPath.empty()) {
    std::fprintf(stderr, "bh_client: use --screenshot or --shot, not both\n");
    return 2;
  }
  if (!shotPath.empty() && maxTicks <= 0) {
    std::fprintf(stderr, "bh_client: --shot requires --max-ticks (exit moment)\n");
    return 2;
  }
  // support "host:port" inside --server
  if (online) {
    const auto colon = server.rfind(':');
    if (colon != std::string::npos) {
      port = std::atoi(server.c_str() + colon + 1);
      server = server.substr(0, colon);
    }
  }

  std::string err;
  auto map = bh::sim::loadBhmap(mapPath, &err);
  if (!map) {
    std::fprintf(stderr, "bh_client: %s\n", err.c_str());
    usage();
    return 1;
  }

  SetTraceLogLevel(LOG_WARNING);
  if (vsync) SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1024, 768, online ? "BLOODHOLLOW - online" : "BLOODHOLLOW - M0");
  SetTargetFPS(60);

  bh::NetClient net;
  if (online) {
    std::string netErr;
    if (!net.connectTo(server, static_cast<std::uint16_t>(port), name, pass, &netErr)) {
      std::fprintf(stderr, "bh_client: connect failed: %s\n", netErr.c_str());
      CloseWindow();
      return 1;
    }
    std::fprintf(stderr, "bh_client: connected to %s:%d as '%s'\n", server.c_str(), port,
                 name.c_str());
  }

  bh::Game game(std::move(*map));
  if (online) game.setOnline(&net);
  bh::TickStepper stepper(bh::sim::kTickSeconds);

  bh::sim::JournalWriter rec;
  if (!recordPath.empty() && !rec.open(recordPath)) {
    std::fprintf(stderr, "bh_client: cannot record to %s\n", recordPath.c_str());
  }
  if (!recordPath.empty() && rec.ok()) game.attachJournalRecorder(&rec);

  bh::sim::Journal journal;
  const bool doReplay = !replayPath.empty();
  std::vector<std::pair<bh::sim::Tick, std::uint64_t>> runHashes;
  if (doReplay) {
    auto j = bh::sim::readJournal(replayPath, &err);
    if (!j) {
      std::fprintf(stderr, "bh_client: %s\n", err.c_str());
      CloseWindow();
      return 1;
    }
    journal = std::move(*j);
    game.attachJournalReplay(&journal);
    game.setHashSink(&runHashes);
  }

  const bool fixedPace = !shotPrefix.empty() || doReplay || maxTicks > 0 || !recordPath.empty();

  bool didWalk = false, didShotDay = false, didDusk = false, didShotNight = false;
  while (!WindowShouldClose()) {
    game.frameBegin();  // online: poll network + send intents; offline: no-op
    if (fixedPace) {
      for (int i = 0; i < kFixedTicksPerFrame; ++i) game.tick();
    } else {
      const double dt = GetFrameTime();
      for (int i = 0, steps = stepper.advance(dt); i < steps; ++i) game.tick();
    }
    // Scripted demo timeline (ticks, not frames - deterministic; latched,
    // because fixed-pace ticks arrive in multiples of kFixedTicksPerFrame).
    if (!shotPrefix.empty()) {
      const auto t = game.tickCount();
      if (t >= 30 && !didWalk) {
        didWalk = true;
        game.commandPath(bh::sim::TilePos{55, 23});
      }
      if (t >= 100 && !didShotDay) {
        didShotDay = true;
        game.render(stepper.alpha());
        TakeScreenshot((shotPrefix + "-day.png").c_str());
      }
      if (t >= 130 && !didDusk) {
        didDusk = true;
        game.debugSetHour(21.8f);
      }
      if (t >= 160 && !didShotNight) {
        didShotNight = true;
        game.render(stepper.alpha());
        TakeScreenshot((shotPrefix + "-night.png").c_str());
      }
    }
    game.render(stepper.alpha());
    const auto t = game.tickCount();
    if (!shotPrefix.empty() && t >= 220) break;
    if (maxTicks > 0 && t >= maxTicks) {
      if (!shotPath.empty()) TakeScreenshot(shotPath.c_str());
      break;
    }
  }

  rec.close();
  CloseWindow();  // NetClient dtor disconnects

  if (online) {
    const bool ok = net.welcomed && game.onlineWelcomed();
    std::fprintf(stderr,
                 "[net] online smoke: welcomed=%d ownId=%u visibleEnts=%zu online=%u "
                 "ping=%dms\n",
                 ok ? 1 : 0, net.ownId, net.ents.size(), net.online, net.pingMs);
    return net.welcomed ? 0 : 4;
  }


  if (doReplay) {
    size_t mismatches = 0;
    const size_t n =
        journal.hashes.size() < runHashes.size() ? journal.hashes.size() : runHashes.size();
    for (size_t i = 0; i < n; ++i) {
      if (journal.hashes[i].second != runHashes[i].second ||
          journal.hashes[i].first != runHashes[i].first) {
        if (mismatches < 5) {
          std::fprintf(stderr, "replay MISMATCH at tick %lld: recorded %llu, replayed %llu\n",
                       static_cast<long long>(journal.hashes[i].first),
                       static_cast<unsigned long long>(journal.hashes[i].second),
                       static_cast<unsigned long long>(runHashes[i].second));
        }
        ++mismatches;
      }
    }
    std::fprintf(stderr, "replay: %zu/%zu tick hashes verified, %zu mismatches\n",
                 n > mismatches ? n - mismatches : 0, n, mismatches);
    return mismatches == 0 ? 0 : 3;
  }
  return 0;
}
