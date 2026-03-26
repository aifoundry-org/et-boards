# ET-BOARD

This repository builds the firmware binary for the ETSOC-1.

Requires et-platform (https://github.com/aifoundry-org/et-platform)
to be installed on your system.

## Installation

To compile:
```
  $ mkdir build && cd build && \
    cmake -DET_PLATFORM=<et-platform-install-dir> -DCMAKE_INSTALL_PREFIX=<install-prefix> ..
  $ make -j$(nproc)
```
where <et-platform-install-dir> is the root where ET-Platform is installed (such as /opt/et).

Once this is complete, you can generate your firmware binaries from the build directory:

```
  $ ./scripts/sign_fw_artifacts.sh
  $ <et-platform-install-dir>/bin/esperanto_flash_tool create \
     	<firmware-name> templates/flash_32Mbit_unsigned.json
```

Where <firmware-name> is the name of the final binary blob, such as 'etsoc1.bin'.


