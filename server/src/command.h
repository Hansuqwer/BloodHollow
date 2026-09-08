#pragma once
// Shared command application (T-049 root-cause fix).
// Both the live network path (tickServer -> processCommand) and the offline
// journal replay (--replay-world) MUST mutate the world through this one
// function.  The pre-T-049 design kept two hand-mirrored switch statements;
// they drifted (kAttack dropped e->path.clear(), kSkill lost the a==0 ->
// attackTarget fallback, kUseItem read q.a instead of channel, kBuy/kTradeItem
// lost the qty>0 guard).  The observed flake: an attack on an already-dead
// target cancelled the walker's queued path live but not in replay, producing
// a transient one-sub-step (256 Q10 units) position slip and a hash mismatch.
// Any future asymmetry here is a replay-classification bug by definition.
#include <cstdint>
#include <string>

#include "content/items.h"
#include "world.h"

namespace bh::server {

struct Command {
  enum Kind : std::uint8_t { kPath, kStep, kChat, kPing, kAttack, kStat,
                             kUseItem, kEquip, kSkill, kBuy, kSellJunk,
                             kTradeOpen, kTradeItem, kTradeGold, kTradeCommit,
                             kTradeCancel, kAnvil,
                             kPartyInvite, kPartyAccept, kPartyLeave,
                             kPartyKick, kKitChoose, kDuel, kForfeit, kRepair, kRefine } kind;
  // T-053: a = kit id; T-056: kDuel a = target id (resolved pre-journal)
  std::int32_t a = 0, b = 0;
  std::uint8_t channel = 0;
  std::string text{};
  std::uint32_t token = 0;
};

// World-mutating command application.  kChat/kPing have no sim effect and are
// handled by the server shell only (never journaled); every other kind routes
// here from BOTH live processing and journal replay.
inline void applyWorldCommand(World& w, Entity& e, const Command& c) {
  switch (c.kind) {
    case Command::kPath:
      w.queuePath(e, sim::TilePos{c.a, c.b});
      break;
    case Command::kStep:
      if (c.a >= -1 && c.a <= 1 && c.b >= -1 && c.b <= 1 &&
          (c.a != 0 || c.b != 0)) {
        const sim::TilePos cur = e.walker.tile();
        e.path.clear();
        e.walker.beginStep(w.gridOf(e), sim::TilePos{cur.x + c.a, cur.y + c.b});  // zone-correct grid (was map/z1)
      }
      break;
    case Command::kAttack:
      // Attack intent cancels manual pathing even when the target turns out to
      // be dead/gone (setAttack then early-returns): the intent arrived, so
      // the walk order is obsolete.
      e.path.clear();
      w.setAttack(e, static_cast<std::uint32_t>(c.a));
      break;
    case Command::kStat:
      w.assignStat(e, c.channel);
      break;
    case Command::kUseItem:
      w.useItem(e, c.channel);
      break;
    case Command::kEquip:
      w.toggleEquip(e, c.channel);
      break;
    case Command::kSkill:
      // a==0 => use current attackTarget (client QoL hotkey).
      w.trySkill(e, c.channel,
                 static_cast<std::uint32_t>(c.a) != 0
                     ? static_cast<std::uint32_t>(c.a)
                     : e.attackTarget);
      break;
    case Command::kBuy:
      // T-069 lane routing: fence stock is item-disjoint from Marta's, so
      // the route is by item (must still stand at the fence — checked in
      // fenceBuy). Deterministic -> replay-exact. Marta's T-056 karma
      // refusal stands for everything that isn't Sable's crate.
      if (c.b > 0) {
        const std::uint32_t itemId = static_cast<std::uint32_t>(c.a);
        if (content::isFenceStock(itemId))
          w.fenceBuy(e, itemId, static_cast<std::uint16_t>(c.b));
        else
          w.vendorBuy(e, itemId, static_cast<std::uint16_t>(c.b));
      }
      break;
    case Command::kSellJunk:
      // T-069: at the gallows end of town the fence pays 60% (no questions);
      // at the plaza Marta pays 40% (and refuses red coin).
      if (w.nearFence(e))
        (void)w.fenceSellJunk(e);
      else
        (void)w.vendorSellJunk(e);
      break;
    case Command::kTradeOpen:
      w.tradeOpen(e, static_cast<std::uint32_t>(c.a));
      break;
    case Command::kTradeItem:
      if (c.b > 0)
        w.tradeOffer(e, static_cast<std::uint32_t>(c.a),
                     static_cast<std::uint16_t>(c.b));
      break;
    case Command::kTradeGold:
      w.tradeOfferGold(e, static_cast<std::uint32_t>(c.a));
      break;
    case Command::kTradeCommit:
      w.tradeCommit(e);
      break;
    case Command::kTradeCancel:
      w.tradeCancel(e, "withdrew");
      break;
    case Command::kAnvil:
      w.tryAnvil(e, c.channel);
      break;
    case Command::kPartyInvite: {
      Entity* t = w.find(static_cast<std::uint32_t>(c.a));  // resolved pre-journal
      if (t != nullptr) w.partyInvite(e, *t);
      break;
    }
    case Command::kPartyAccept:
      w.partyAccept(e);
      break;
    case Command::kPartyLeave:
      w.partyLeave(e);
      break;
    case Command::kPartyKick:
      w.partyKick(e, static_cast<std::uint32_t>(c.a));
      break;
    case Command::kKitChoose:
      w.kitChoose(e, static_cast<std::uint8_t>(c.a));
      break;
    case Command::kDuel:
      w.duelChallenge(e, static_cast<std::uint32_t>(c.a));
      break;
    case Command::kForfeit:
      w.duelForfeit(e);
      break;
    case Command::kRepair:
      w.repairAll(e);  // T-058: vendor-proximity gold toll
      break;
    case Command::kRefine:
      w.tryRefine(e, static_cast<std::uint8_t>(c.a));  // T-060 anvil upgrade
      break;
    case Command::kChat:
    case Command::kPing:
      break;  // no world effect; server-shell handled
  }
}

}  // namespace bh::server
