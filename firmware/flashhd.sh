#!/bin/bash
echo "Usage: $0 /dev/ttyUSBx os6_hd.img"

python $HOME/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port $1 --baud $((921600/2)) --before default_reset --after hard_reset write_flash --flash_mode qio --flash_freq 80m --flash_size detect 0x140000 $2
