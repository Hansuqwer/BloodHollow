#!/bin/bash
pkill -x bh_server 2>/dev/null; pkill -x bh_bots 2>/dev/null; sleep 1
rm -f /tmp/s9g.db /tmp/s9g.bwj /tmp/s9g_srv.log
./build/server/bh_server --db /tmp/s9g.db --port 7814 --soak-secs 220 --record-world /tmp/s9g.bwj \
  --bless bot_00=2001:1,4001:35,skill:25,gold:400 \
  --bless bot_01=2001:1,4001:35,skill:25,gold:400 \
  --bless bot_02=2002:1,4002:16,4001:70,skill:60,gold:1100 \
  --bless bot_03=2002:1,4002:16,4004:12,4001:70,skill:90,gold:2200 \
  --bless bot_04=2001:1 --bless bot_05=2001:1 \
  > /tmp/s9g_srv.log 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port 7814 --profile pilgrim --count 6 --secs 190 2>/dev/null | grep SUMMARY
wait $SRV
