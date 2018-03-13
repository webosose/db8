#!/bin/bash

# This script register fault when I/O error occur on main db process
# only for QSM+ mode

mkdir -p /mnt/lg/cmn_data/db8 || true
touch /mnt/lg/cmn_data/db8/registerFault
sync

/usr/bin/luna-send -n 1 luna://com.webos.service.faultmanager/registerAndNotifyFault '{"faultReason":"dbError", "pid":"mojodb-pid", "pname":"mojodb-pname"}'
