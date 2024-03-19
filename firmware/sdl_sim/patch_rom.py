from sys import argv


def patch(dat, base, int_val, length, byteorder='big'):
	bs_val = int.to_bytes(int_val, length=length, byteorder=byteorder)
	for i, b in enumerate(bs_val):
		dat[base + i] = b


def update_check_sum(dat):
	oldchs = (dat[0] << 24) + (dat[1] << 16) + (dat[2] << 8) + (dat[3])
	newchs = 0

	for i in range(4, len(dat), 2):
		newchs += (dat[i] << 8) + dat[i + 1]

	print(f"ROM size:     {len(dat):8x}")
	print(f"Old checksum: {oldchs:8x}")
	print(f"New checksum: {newchs:8x}")

	patch(dat, 0, newchs, 4)


def main():
	if (len(argv) < 3):
		print('usage:', argv[0], 'input.rom output.rom')
		exit(-1)

	with open(argv[1], 'rb') as f:
		dat = bytearray(f.read())

	patch(dat, 0x02ae, 0x00400000, 4, 'little')

	update_check_sum(dat)

	with open(argv[2], 'wb') as f:
		f.write(dat)
	print('wrote', argv[2])


if __name__ == '__main__':
	main()
