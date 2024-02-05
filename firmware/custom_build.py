import os
Import("env")

# 68k code generation
# env.Execute("make -C components/tme autogen")

# upload rom and hdd image to flash
config = env.GetProjectConfig()
options = ['rom', 'hdd']
addresses = [0x100000, 0x120000]

for opt, adr in zip(options, addresses):
    bin_name = config.get('env:minimac', f'custom_{opt}_name', None)
    if bin_name is not None:
        print(f"Flashing {bin_name} at 0x{adr:08x}")
        if not os.path.exists(bin_name):
            raise FileNotFoundError(bin_name)
        env.Append(FLASH_EXTRA_IMAGES=[(bin_name, adr)])
