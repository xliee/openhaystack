@echo off
@REM flash.bat "C4XtRo98PhhzO9V/jwwIPprCjxk3mSn+O+gRIG7unWc=" "s9J5hI/DxtLlG31A41zil+wKflRixSLKx2M8yA=="

set sk_path=sk.key
set pk_path=pk.key

@REM C4XtRo98PhhzO9V/jwwIPprCjxk3mSn+O+gRIG7unWc=
set skb64=%1

@REM s9J5hI/DxtLlG31A41zil+wKflRixSLKx2M8yA==
set pkb64=%2

echo "%skb64%"
echo "%pkb64%"

echo "%skb64%" | python -m base64 -d - > %sk_path%
echo "%pkb64%" | python -m base64 -d - > %pk_path%


esptool.py --after no_reset -p COM6 erase_region 0xa000 0x5000

esptool.py --before no_reset --baud 115200 -p COM6 write_flash ^
    0x1000  "build/bootloader/bootloader.bin" ^
    0x9000  "build/partition_table/partition-table.bin" ^
    0xd000  "%sk_path%" ^
    0xe000  "%pk_path%" ^
    0x10000 "build/openhaystack.bin"
