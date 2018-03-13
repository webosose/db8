#!/bin/bash
#clear database
PmLogCtl log DB8 crit "mojodb-luna [] DB8 DBGMSG {} [errorOpenEpgDb.bash] Reinitialize the EPG DB."
rm -rf /var/db/epg/*
