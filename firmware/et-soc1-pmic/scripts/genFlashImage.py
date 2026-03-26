#!/usr/bin/env python3

from argparse import ArgumentParser
from os.path import getsize, join, exists
from os import getcwd
from pathlib import Path
from common_utils import addMetadata

###################################################################
#                - Flash Image Layout (256KB) -                   #
#                      (Base Address: 0x0)                        #
#     - user              - base-offset      - size               #
#     Bootloader          0x0                0x2000  (8KB)        #
#     FW Image (Slot 0)   0x2000             0x1E800 (122KB)      #
#     FW Image (Slot 1)   0x20800            0x1E800 (122KB)      #
#     Non Volatile Memory 0x3F000            0x1000  (4KB)        #
###################################################################
# OR
###################################################################
#                - Flash Image Layout Standalone(256KB) -         #
#                      (Base Address: 0x0)                        #
#     - user              - base-offset      - size               #
#     FW Image            0x0                0x1E800 (122KB)      #
#     Non Volatile Memory 0x3F000            0x1000  (4KB)        #
###################################################################

# Constants
FLASHSIZE = (256<<10)                     # Size of total flash
BLSIZE = (8<<10)                          # Bootlader Image size
REAL_FWSIZE = (122<<10)                   # PMIC FW Real Image size
SLOT0_FWSIZE = REAL_FWSIZE                # PMIC FW slot 0 Image size
SLOT1_FWSIZE = SLOT0_FWSIZE + REAL_FWSIZE # PMIC FW slot 1 Image size
CONFIG_HEADER_SIZE = 256                  # Config header section size (part of non-volatile memory)

def main():

  parser = ArgumentParser(description='Create flash binary image with provided parameters.')
  parser.add_argument('-o', '--output', default='flash-image.bin', help='Output flash image name')
  parser.add_argument('-d', '--directory', default=getcwd(), help='Output directory path')
  parser.add_argument('-b', '--bootloader', default='', help='Path to bootloader binary image')
  parser.add_argument('-s0', '--slot0', default='', help='Path to PMIC FW slot0 binary image')
  parser.add_argument('-s1', '--slot1', default='', help='Path to PMIC FW slot1 binary image')
  parser.add_argument('-s2', '--standalone', default='', help='Path to PMIC FW standalone binary image')
  parser.add_argument('-m', '--metadataOffset', type=int, required=True, help='Offset of the image metadata.')

  args = parser.parse_args()

  assert exists(args.directory)
  assert (args.slot0 != '') or (args.slot1 != '') or (args.standalone != '')
  assert (args.metadataOffset < REAL_FWSIZE)
  # If bootloader image is not specified, standalone image is generated, otherwise slot0 or slot 1 images must be specified
  if args.bootloader == '':
    assert (args.standalone != '')
    assert (args.slot0 == '') and (args.slot1 == '')  

  # Standalone image
  standaloneData = bytearray()
  if not args.standalone == '':
    assert exists(args.standalone)
    assert getsize(args.standalone) <= REAL_FWSIZE
    standalone_start = 0
    print("Firmare standalone Image start offset from application start address: " + str(hex(standalone_start)))

    # Create byte stream of firmware
    with open(args.standalone, 'rb') as fo:
        fo.seek(0)
        standaloneData = bytearray(fo.read())
        fo.close()
    assert len(standaloneData) >= standalone_start

    # Update image metadata
    standaloneData = addMetadata(standaloneData, args.metadataOffset)
    # Fill the remaining size area with zeros
    nulldata = bytearray([0] * (2*REAL_FWSIZE + BLSIZE - len(standaloneData)))
    standaloneData.extend(nulldata)

  # Slot 0 image
  slot0data = bytearray()
  if not args.slot0 == '':
    assert exists(args.slot0)
    assert getsize(args.slot0) <= SLOT0_FWSIZE
    assert exists(args.bootloader)
    slot0_start = 0
    print("Firmare slot0 Image start offset from application start address: " + str(hex(slot0_start)))

    # Create byte stream of firmware
    with open(args.slot0, 'rb') as fo:
        fo.seek(0)
        slot0data = bytearray(fo.read())
        fo.close()
    assert len(slot0data) >= slot0_start

    # Update image metadata
    slot0data = addMetadata(slot0data, args.metadataOffset)
    # Fill the remaining size + slot 1 area with zeros
    nulldata = bytearray([0] * (REAL_FWSIZE - len(slot0data)))
    slot0data.extend(nulldata)

  # Slot 1 image
  slot1data = bytearray()
  if not args.slot1 == '':
    assert exists(args.slot1)
    assert getsize(args.slot1) <= SLOT1_FWSIZE
    assert exists(args.bootloader)
    slot1_start = REAL_FWSIZE
    print("Firmare slot1 Image start offset from application start address: " + str(hex(slot1_start)))

    # Create byte stream of firmware
    with open(args.slot1, 'rb') as fo:
        fo.seek(0)
        slot1data = bytearray(fo.read())
        fo.close()
    assert len(slot1data) >= slot1_start

    # Strip the 122K for FW image (empty slot 0)
    slot1data = slot1data[slot1_start:]
    # Update image metadata
    slot1data = addMetadata(slot1data, args.metadataOffset)
    # Fill the remaining size and slot 0 memory area with zeros
    nulldata = bytearray([0] * (REAL_FWSIZE - len(slot1data)))
    slot1data.extend(nulldata)

  # Bootloader image
  bootdata = bytearray()
  if not args.bootloader == '':
    # Create byte stream of bootloader
    with open(args.bootloader, 'rb') as fo:
        fo.seek(0)
        bootdata = bytearray(fo.read())
        fo.close()
    # Fill the remaining size with zeros
    nulldata = bytearray([0] * (BLSIZE - getsize(args.bootloader)))
    bootdata.extend(nulldata)

  # Create config header byte stream and initialize it to default values (boot slot = selected slot, boot count = 0)
  configHeader = bytearray()
  if args.slot0 != '':
      configHeader = bytearray([0, 0, 0, 0, 0, 0, 0, 0])
  elif args.slot1 != '':
      configHeader = bytearray([1, 0, 0, 0, 0, 0, 0, 0])
  nulldata = bytearray([0] * (CONFIG_HEADER_SIZE - 8))
  configHeader.extend(nulldata)

  # Check and create output flash image
  # Bootloader = 8 KB OR not present for standalone image
  # Firmware (two copies OR only one for standalone) = 122 KB
  # Initial config header (not part of the application firmware, initialized only with this script)
  Path(args.directory).mkdir(parents=True, exist_ok=True)
  emptySlot = bytearray([0] * (REAL_FWSIZE))
  with open(join(args.directory, args.output), 'wb') as out:
    if args.slot0 != '' and args.slot1 != '':
      out.write(bootdata + slot0data + slot1data + configHeader)
    elif args.slot0 != '':
      out.write(bootdata + slot0data + emptySlot + configHeader)
    elif args.slot1 != '':
      out.write(bootdata + emptySlot + slot1data + configHeader)
    elif args.standalone != '':
      out.write(standaloneData)
    out.close()
    assert getsize(join(args.directory, args.output)) <= FLASHSIZE
    print("Flash image available at: " + str(join(args.directory, args.output)))

if __name__ == "__main__":
  main()