#!/bin/bash
echo "Usage: $0 os6_hd.img"

python $HOME/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 write_flash 0x204000 $1
