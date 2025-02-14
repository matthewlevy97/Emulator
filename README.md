# TL;DR
A WIP multi-platform emulator currently supporting the Chip8 interpreter and subset of GameBoy capabilities

# Approach to Modular Retro-Emulation
Old school systems were very simple and therefore do not contain a lot of the complexities that modern systems have. If there components could be abstracted out at a high level, systems could utilize these building blocks to quickly achieve accurate emulation.

Everything here is broken down into [components](emulator/components), enabling a heavy "plug-n-play" environment for emulation. Need some ReadOnly memory, create a new memory object, tell it the address range and size to handle, then attach it to the bus. Need to draw pixels to the screen, create a Display object and put the pixels into the correct position. Drawing to the screen is already taken care of.

This approach allows creating a [Chip8 interpreter](emulator/systems/chip8) in ~700 lines of normally written code.

# Supported Systems
- Chip8
- Nintendo GameBoy
