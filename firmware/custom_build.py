Import("env")

# 68k code generation
env.Execute("make -C src/musashi")
