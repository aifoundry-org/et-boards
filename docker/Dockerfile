FROM et-platform-soc-firmware AS soc-firmware
FROM ubuntu:24.04

ENV ET=/opt/et

WORKDIR /w

RUN apt update -y
RUN apt install -y build-essential git cmake gcc-arm-none-eabi python3-xxhash python3-numpy libjson-c-dev libssl-dev

COPY --from=soc-firmware ${ET} ${ET}

# build the bits
COPY firmware . 
RUN mkdir build
RUN cd build && cmake -DET_PLATFORM=${ET} -DCMAKE_INSTALL_PREFIX=${ET} ..
RUN cd build && make -j $(nproc) 

# put the bits where they belong
RUN cp build/scripts/* ${ET}/bin
RUN mkdir -p ${ET}/share/flash_tool
RUN cp build/templates/* ${ET}/share/flash_tool
