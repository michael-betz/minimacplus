#!/bin/bash
echo "Usage: $0 mac_plus_rom.bin"

python $HOME/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 write_flash 0x1e4000 $1

