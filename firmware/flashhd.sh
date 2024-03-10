#!/bin/bash
if [ -n "$1" ]; then
	echo "Usage: $0 mac_plus_rom.bin /dev/ttyUSBx"
fi

PORT=/dev/ttyUSB0
FILE=../mame/hd.img
if [ -n "$1" ]; then PORT=$1; fi
if [ -n "$2" ]; then FILE=$2; fi

python $HOME/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port $PORT --baud $((921600/2)) --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x140000 $FILE

