# ET-BOARDS

This repository contains schematics, BOMs and software required for
various ICs in the BOM for various boards built around ET silicon.

For now, all of the software is under firmware/ and we hope to split
it into nice groupings over time. If you want to build the software
you need et-platform (https://github.com/aifoundry-org/et-platform)
to be installed on your system.

## Required toolchains

The software under firmware/et-soc1-pmic requires a [vendor provided](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
toolchain for the ARM compiler. While it builds with a stock gcc
gcc-arm-none-eabi the resulting binary won't boot.
