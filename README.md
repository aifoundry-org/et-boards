# Alien Board

Alien Board is an open source single-board-computer design around
Esperanto ET-SoC-1 that can support stand-alone operation,
including remote operation over WiFi/Ethernet and desktop-style operation.

```mermaid
flowchart LR
    %% Core SoC
    SoC[ET-SoC-1 SoC]

    %% Memory
    SoC --> LPDDR[32 GB LPDDR4X]
    SoC --> EMMC[64 GB eMMC]

    %% PCIe root to switch
    SoC -->|PCIe Gen4 x8| SW[Microchip Switchtec PFX PCIe Gen4 Switch PM40052]

    %% Storage: two NVMe SSDs
    SW -->|x4| M2A[M.2 M-key 1 NVMe SSD]
    SW -->|x4| M2B[M.2 M-key 2 NVMe SSD]

    %% Wireless M.2 slot (WiFi/BT)
    SW -->|x1| M2W[M.2 E-key WiFi BT]

    %% Dual 10G SFP+ via PCIe NIC / MAC+PHY
    SW -->|x4| NIC[Dual 10GbE MAC PHY]
    NIC --> SFP1[SFP+ Cage 1 10GBASE-R]
    NIC --> SFP2[SFP+ Cage 2 10GBASE-R]

    %% USB3 via PCIe host controller + hub
    SW -->|x2| USBHC[PCIe USB3 Host Controller]
    USBHC --> USBHUB[USB3 Hub]
    USBHUB --> USB[USB3 Host Ports]

    %% Display via PCIe display controller to DP++ (HDMI via adapter)
    SW -->|x4| GPU[PCIe Display Controller]
    GPU --> DP[DisplayPort++ Connector HDMI via adapter]

    %% High-speed lanes broken out to SYZYGY
    SW -->|x4| SYZ_HS[SYZYGY High-Speed Expansion Ports]

    %% Low-speed I/O fan-out
    SoC --> LSIO[Low-Speed IO I2C SPI UART GPIO]
    LSIO --> mikro1[mikroBUS Shuttle 1]
    LSIO --> mikro2[mikroBUS Shuttle 2]
    LSIO --> mikro3[mikroBUS Shuttle 3]

    %% SYZYGY control / sideband
    LSIO --> SYZ_LS[SYZYGY Control I2C GPIO]
    SYZ_LS --> SYZ_HS

    %% Debug interfaces
    SoC --> SOICBite[16-pin SOICBite JTAG UART]
    SoC --> PiProbe[3-pin Pi Debug Probe UART]

    %% USB2 device over USB-C for networking-over-USB
    SoC --> USB2D[USB2 Device Controller]
    USB2D --> USBC_DEV[USB-C Device Port RNDIS ECM]
```

## Block Overview

Alien Board is built around the Esperanto **ET-SoC-1** device, exposing its PCIe Gen4 x8 interface and low-speed I/O into a flexible high-speed expansion platform.

### Compute and Memory

- **ET-SoC-1 SoC** – primary compute, AI, and control engine.
- **32 GB LPDDR4X** – main system memory. (Let us max-out the local memory.)
- **64 GB eMMC** – non-volatile storage for bootloader, OS, and base software image. (This is largely just for boot with bulk data living on the SSDs, but also for an "out-of-box" experience.)

### High-Speed PCIe Fabric

A **Microchip Switchtec PFX PM40052 (52-lane, PCIe Gen4)** switch receives the SoC’s PCIe Gen4 x8 link and fans it out into multiple downstream ports: (We can probably find a cheaper switch, but we want to make sure to get good utilization of this primary mechanism for high-speed I/O.)

- **2× M.2 M-key NVMe SSD slots** (typically x4 each) (It is critical to have high-speed local storage under control of the SoC.)
- **1× M.2 E-key slot** for WiFi/Bluetooth (x1) (Best to have a WiFi option.)
- **Dual-port 10GbE NIC** connected to:
  - **2× SFP+ cages** for 10GBASE-R optical/DAC modules
- **PCIe USB3 host controller** feeding a USB3 hub and multiple USB3 ports (USB3 is great for off-the-shelf cameras and also enables keyboards/mice and video adapters.)
- **PCIe display controller** providing DisplayPort++ (HDMI via passive adapter) (There should be some kind of out-of-box display option for the keyboard/monitor/mouse types.)
- **Remaining high-speed lanes routed to [SYZYGY](https://syzygyfpga.io/) high-speed connectors**
  for custom high-performance expansion  (May stuggle with this due to the SERDES being locked down to PCIe.)

### Expansion, I/O and Debug

- **3× mikroBUS shuttle connectors** using I²C, SPI, UART, and GPIO signals.
- **SYZYGY high-speed connectors** for DAQ, networking, sensor front-ends, and custom pods with
  I²C + GPIO for configuration, presence detect, and power.
- **USB3 host ports** via PCIe USB3 controller + hub.
- **USB2 device port (USB-C)** for BeagleBoard-style “networking over USB” workflows:
  - RNDIS/ECM Ethernet gadget
  - USB serial console
  - Optional mass-storage gadget
- **16-pin SOICBite JTAG/UART connector** for bring-up and debug.
- **3-pin Pi Debug Probe UART header** for simple serial access.

## Example Applications

Alien Board is intended as a flexible “edge accelerator” and experimentation platform for high-speed, high-I/O embedded systems. Some example use cases:

### Stand-Alone Large Language Models and Smart Speakers
- Run medium-to-large language models entirely offline.
- Build a smart speaker powered by local inference, no cloud required.
- Pair with onboard WiFi or SFP+ for integration into home automation.

### On-Device AI for Drones and Robotics
- Use ET-SoC-1 as the AI perception and planning brain.
- Bring in cameras or sensors over SYZYGY or USB3.
- Perform SLAM, obstacle avoidance, and control loops directly on-board.

### High-Speed Data Acquisition and Instrumentation
- Use SYZYGY or PCIe endpoints to attach high-speed ADC/DAC modules.
- Stream multi-gigabit data directly to NVMe.
- Perform local signal processing, FFTs, ML-based anomaly detection.
- Ideal for software-defined radio, radar/lidar prototyping, industrial sensing, or lab automation.

### Hardware-in-the-Loop (HIL) and Automated Test Systems
- Integrate with production-test ecosystems like power controllers and relay boards.
- Use Alien Board as a fully programmable tester:
  - power cycling
  - firmware loading
  - signal capture and analysis
  - automated pass/fail criteria
- High-speed links allow real-time capture and logging to NVMe.
- Imagine [Blue Clover test jigs](https://bcdevices.com) and [BootLoop code generation](https://bootloop.ai)

### TV Monitoring and Content Filtering
- Capture HDMI or DisplayPort input via add-on pods.
- Use on-device AI to detect commercials, objectionable content, or logos.
- Output filtered or annotated content over HDMI/DP.
- Ideal for parental controls, ad detection, or accessibility overlays.

### Fully Open-Source GPU / Compute Pipelines
- Combine ET-SoC-1’s AI compute with open-source-friendly PCIe display devices.
- Use PCIe display controller only for scan-out, while ET-SoC-1 handles heavy rendering and ML tasks.
- Explore open graphics stacks, Vulkan compute, shader prototyping, etc.

These examples are intended as starting points. The board is deliberately over-provisioned on PCIe lanes and connectors to support experimental pods and novel high-speed interfaces.

## Form-factor

Alien Board should target something like a NUC form-factor, which is about 10 x 10 cm (4 x 4 inches).

![](https://upload.wikimedia.org/wikipedia/commons/thumb/4/48/Intel_NUC_DCCP847DYE.jpg/250px-Intel_NUC_DCCP847DYE.jpg)

```mermaid
flowchart TB
    %% Front or side edge: user I/O
    subgraph Front_Edge[Front / Side I/O Edge]
        DP[DP++ / HDMI Out]
        USBC_HOST[USB3 Host Ports]
        USBC_DEV[USB-C USB2 Device]
        SFP1[SFP+ Cage 1]
        SFP2[SFP+ Cage 2]
    end

    %% Board core: SoC, memory, switch
    subgraph Board_Core[Board Core]
        SoC[ET-SoC-1]
        LPDDR[32 GB LPDDR4X]
        EMMC[64 GB eMMC]
        SW[Switchtec PM40052 PCIe Gen4 Switch]
        NIC[Dual 10GbE MAC PHY]
        USBHC[PCIe USB3 Host]
        GPU[PCIe Display Controller]
    end

    %% Top / interior: M.2 and SYZYGY, mikroBUS, debug
    subgraph Top_Side[Top Side: M.2 / SYZYGY / Expansion]
        M2A[M.2 M-key NVMe 1]
        M2B[M.2 M-key NVMe 2]
        M2W[M.2 E-key WiFi BT]
        SYZ1[SYZYGY Port A]
        SYZ2[SYZYGY Port B]
        mikro1[mikroBUS Shuttle 1]
        mikro2[mikroBUS Shuttle 2]
        mikro3[mikroBUS Shuttle 3]
        SOICBite[16-pin SOICBite JTAG UART]
        PiProbe[3-pin Pi Debug UART]
    end

    %% Connections between regions
    SoC --> LPDDR
    SoC --> EMMC
    SoC --> SW

    SW --> NIC
    SW --> USBHC
    SW --> GPU
    SW --> M2A
    SW --> M2B
    SW --> M2W
    SW --> SYZ1
    SW --> SYZ2

    NIC --> SFP1
    NIC --> SFP2

    USBHC --> USBC_HOST
    GPU --> DP

    SoC --> USBC_DEV
    SoC --> mikro1
    SoC --> mikro2
    SoC --> mikro3
    SoC --> SOICBite
    SoC --> PiProbe
```
