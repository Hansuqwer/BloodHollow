# BLOODHOLLOW wire protocol v0 (ADR-006)

Single source of truth for the network format. `tools/protogen/protogen.py`
generates the serializers; `kProtocolVersion` bumps on any incompatible change.
Convention: ids 1..63 client->server, 100+ server->client. One message per
packet, framed as `[u16 id][u16 payloadLen][payload]`.

## Client -> Server

First packet after connect. Wrong `protoVersion` => LoginResult(ok=0, reason=4).

```proto
message Hello = 1
u16 protoVersion
string username
string password
```

```proto
message InputPath = 3
i32 goalX
i32 goalY
```

```proto
message InputStep = 4
i8 dx
i8 dy
```

```proto
message ChatSend = 5
u8 channel
string text
```

```proto
message Ping = 6
u32 clientTimeMs
```

Combat intent: keep swinging at targetId until it dies / leaves AoI / a path or
step command cancels. Equal-range chase is server-authoritative.

```proto
message AttackRequest = 7
u32 targetId
```

Spend one unassigned stat point (kStatPointsPerLevel per level-up).
stat: 0=STR, 1=VIT, 2=DEX, 3=INT, 4=MAG (T-160 five-stat model, wave-2).

```proto
message StatAssign = 8
u8 stat
```

```proto
message UseItem = 9
u8 slot
```

```proto
message ToggleEquip = 10
u8 slot
```

skill 1 = Power Swing (140% weapon, 40t cooldown).

```proto
message SkillUse = 11
u8 skill
u32 targetId
```

Vendor trade: buy from town vendor stock (must stand within 3 tiles).

```proto
message BuyRequest = 12
u32 itemId
u16 qty
```

One-shot: sell every junk-slot item at 40% of value (MVP pawn path).

```proto
message SellJunk = 13
u8 unused
```

## Trade window (S7): open with adjacent player, offer items/gold, both commit = swap.

```proto
message TradeOpen = 14
u32 targetId
```

```proto
message TradeOfferItem = 15
u32 itemId
u16 qty
```

```proto
message TradeOfferGold = 16
u32 gold
```

```proto
message TradeCommit = 17
u8 unused
```

```proto
message TradeCancel = 18
u8 unused
```

Anvil ceremony: request tier attempt at an adjacent anvil (RFC 0001).

```proto
message AnvilOp = 19
u8 tier
```


Party commands. Slash forms in chat (/invite <name>, /accept, /leave, /kick
<name>) are parsed server-side into these same intents.

```proto
message PartyInvite = 21
string name
```

```proto
message PartyAccept = 22
u8 unused
```

```proto
message PartyLeave = 23
u8 unused
```

```proto
message PartyKick = 24
u32 targetId
```

T-167 creation (1-char alpha): a fresh account's first Hello gets LoginResult
ok=1 AND a CharCreatePrompt instead of a spawn; the client answers CharCreate
once (class 1..3, sex 1=m/2=f) and the server spawns. New messages bump the
version via the count (no base change needed for additions).

```proto
message CharCreate = 25
u8 classId
u8 sex
```

## Server -> Client

```proto
message LoginResult = 100
u8 ok
u8 reason
```

```proto
message Welcome = 101
u32 entityId
u32 mapId
i32 x
i32 y
u32 tick
u32 hourCenti
```

```proto
message EntitySpawn = 102
u32 id
u8 kind
u8 dir
i32 x
i32 y
u32 hp
u32 hpMax
u8 level
string name
u8 karmaBand
u8 light
u8 glowTier  // T-092: equipped-weapon refine glow (0 none, 1 +5..9, 2 +10+)
u8 classId  // T-142: kit id (kits.h; 0 unsworn, 1 Ravager, 2 Gravecaller, 3 Cultist)
u8 sex  // T-142: 0 unknown (T-142b captures), 1 m, 2 f
```

```proto
message EntityDelta = 103
u32 id
i32 x
i32 y
u8 dir
u8 moving
u32 hp
u8 light
u8 glowTier  // T-092: rides the delta (refine/repair/equip change it live)
u8 classId  // T-142: rides the delta so a /kit oath re-sheets remotes live
u8 sex  // T-142: immutable once captured (T-142b); 0 until then
```

```proto
message EntityDespawn = 104
u32 id
```

```proto
message ChatMsg = 105
u8 channel
string from
string text
```

```proto
message Pong = 106
u32 clientTimeMs
u32 serverTick
```

```proto
message KickNotice = 107
string reason
```

```proto
message ServerStats = 108
u32 online
u32 tickMicrosP99
```

combat kind: 0=miss, 1=hit, 2=crit, 3=kill (victim is targetId; amount = overkill
dmg), 4=heal (amount healed, target healed), 5=skill hit (Power Swing)

```proto
message CombatEvent = 109
u32 attackerId
u32 targetId
u8 kind
u16 amount
```

Pushed on login, level-up, and stat assignment. intg/mag are 0 until casters exist.

```proto
message OwnStats = 110
u16 level
u32 xp
u32 xpNext
u8 statPoints
u8 str
u8 vit
u8 dex
u8 intg
u8 mag
u16 swordSkill
u32 gold
i32 karma
u8 classId
u32 mp
u32 mpMax
u16 blessTicksLeft
u16 ironskinTicksLeft
u16 curseTicksLeft
```

Inventory sync: InventoryReset wipes the client table (carries gold), then one
ItemSlot per occupied slot follows on the same reliable channel.

```proto
message InventoryReset = 111
u8 count
```

```proto
message ItemSlot = 112
u8 slot
u32 itemId
u16 qty
u8 equipped
u8 aura
u8 durability
u8 affix
u8 refine
u8 rarity  // T-159: 0 common, 1 magic, 2 rare, 3 unique (bag-row marker + name colour)
u8 affix2  // ADR-0016: 0 none (Magic 50%, Rare always)
u8 affix3  // ADR-0016: 0 none (Rare 50%)
```

All i32 positions are Q10 fixed-point tile coordinates (sim::kUnitsPerTile=1024).
Chat channels: 0=say (AoI radius), 1=global, 2=system (server-originated),
3=death notices (server-originated).

Entity kinds: 0=player; 1..63 = mob (index+1 into content/kMobs); 64 = vendor NPC.

Party roster sync: PartyReset wipes the client table (partyId=0 == left party),
then one PartyMember per roster entry follows on the same reliable channel.
zoneId lets the frame grey out off-map members.

```proto
message PartyReset = 115
u32 partyId
u32 leaderId
u8 count
```

```proto
message PartyMember = 116
 u32 entityId
 string name
 u16 level
 u32 hp
 u32 hpMax
 u16 zoneId
```

Siege + pledge wire (T-151, epoch 28): SiegeState carries the castle memory
(window/battle ticks, gate HP[2] max 300, heart progress/attuned, crown channel,
band count, phase, vault/crowns, holder pledge). PledgeRoster is the guild
sheet (header + per-member rows, inventory-style).

```proto
message SiegeState = 117
 u32 holderPledgeId
 string holderPledgeName
 string holderName
 u32 windowEndTick
 u32 battleEndTick
 u16 gateHp0
 u16 gateHp1
 u16 heartProgress
 u8 heartAttuned
 u32 crownOwnerId
 string crownOwnerName
 u32 crownDeadline
 u8 bandCount
 u8 phase
 u32 vaultGold
 u32 crowns
```

```proto
message PledgeRoster = 118
 u32 pledgeId
 string name
 u8 emblem
 u8 count
 u32 vaultGold
```

```proto
message PledgeMember = 119
 string name
 u8 rank
 u16 level
 u8 online
```

```proto
message CharCreatePrompt = 120
u8 unused
```
