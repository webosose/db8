#!/bin/bash

# This script is executed by db8 maindb upstart script

#do factory reset
mkdir -p /mnt/lg/cmn_data/db8 || true
touch /mnt/lg/cmn_data/db8/errorNoSpace || true
rm -f /var/luna/preferences/ran-firstuse || true
rm -rf /var/db/main/* || true
sync

PmLogCtl log DB8 crit "mojodb-luna [] DB8 DBGMSG {} [upstart_maindb] No space left for maindb"
/usr/bin/luna-send -n 1 luna://com.webos.service.tv.systemproperty/doUserDefault '{}'
