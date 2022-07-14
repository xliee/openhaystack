@echo off
@REM flash.bat COM6 "C4XtRo98PhhzO9V/jwwIPprCjxk3mSn+O+gRIG7unWc=" "BBX8cYk9jDdMRRuty8kTyJHelSPhYAfgGDMI4Kb88Nl9PTshm7nBZY1/FowHEHJ6b61Y7HkXhSdh"

set sk_path=sk.key
set pk_path=pk.key

set PORT=%1

@REM SIMETRIC KEY e.g: C4XtRo98PhhzO9V/jwwIPprCjxk3mSn+O+gRIG7unWc=
set skb64=%2

@REM PUBLIC KEY e.g: BBX8cYk9jDdMRRuty8kTyJHelSPhYAfgGDMI4Kb88Nl9PTshm7nBZY1/FowHEHJ6b61Y7HkXhSdh
set pkb64=%3

echo "%skb64%"
echo "%pkb64%"

echo "%skb64%" | python -m base64 -d - > %sk_path%
echo "%pkb64%" | python -m base64 -d - > %pk_path%


esptool.py --after no_reset -p %PORT% erase_region 0xa000 0x5000

esptool.py --before no_reset -p %PORT% --baud 115200 write_flash ^
    0x1000  "build/bootloader/bootloader.bin" ^
    0x9000  "build/partition_table/partition-table.bin" ^
    0xe000  "%sk_path%" ^
    0xf000  "%pk_path%" ^
    0x10000 "build/openhaystack.bin" ^
