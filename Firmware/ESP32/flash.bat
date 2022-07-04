esptool.py --before no_reset --baud 115200 -p COM6 write_flash 0x1000 "build/bootloader/bootloader.bin"  0x8000  "build/partition_table/partition-table.bin"  0x10000 "build/openhaystack.bin"
