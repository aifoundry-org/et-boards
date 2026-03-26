#!/usr/bin/env python3

from argparse import ArgumentParser
from os.path import getsize, join, exists
from os import getcwd
from pathlib import Path
from common_utils import addMetadata

# Constants
FLASHSIZE = (256<<10)                     # Size of total flash
BLSIZE = (8<<10)                          # Bootlader Image size
REAL_FWSIZE = (122<<10)                   # PMIC FW Real Image size
SLOT0_FWSIZE = REAL_FWSIZE                # PMIC FW slot 0 Image size
SLOT1_FWSIZE = SLOT0_FWSIZE + REAL_FWSIZE # PMIC FW slot 1 Image size

def main():

  parser = ArgumentParser(description='Create a stripped down binary slot image for firmware update')
  parser.add_argument('-o', '--output', default='slot-image.bin', help='Output flash image name')
  parser.add_argument('-d', '--directory', default=getcwd(), help='Output directory path')
  parser.add_argument('-s0', '--slot0', default='', help='Path to PMIC FW slot0 binary image')
  parser.add_argument('-s1', '--slot1', default='', help='Path to PMIC FW slot1 binary image')
  parser.add_argument('-m', '--metadataOffset', type=int, required=True, help='Offset of the image metadata.')

  args = parser.parse_args()

  assert exists(args.directory)
  assert (args.slot0 != '') or (args.slot1 != '')
  assert (args.metadataOffset < REAL_FWSIZE)

  slot0_start = 0
  slot0data = bytearray()
  if not args.slot0 == '':
    assert exists(args.slot0)
    assert getsize(args.slot0) <= SLOT0_FWSIZE
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

  slot1_start = 0
  slot1data = bytearray()
  if not args.slot1 == '':
    assert exists(args.slot1)
    assert getsize(args.slot1) <= SLOT1_FWSIZE
    slot1_start = REAL_FWSIZE
    print("Firmware slot1 Image start offset from application start address: " + str(hex(slot1_start)))

    # Create byte stream of firmware
    with open(args.slot1, 'rb') as fo:
        fo.seek(0)
        slot1data = bytearray(fo.read())
        fo.close()
    assert len(slot1data) >= slot1_start

    # Strip the 122K for FW image
    slot1data = slot1data[slot1_start:]

    # Update image metadata
    slot1data = addMetadata(slot1data, args.metadataOffset)

  # Create slot image
  Path(args.directory).mkdir(parents=True, exist_ok=True)
  with open(join(args.directory, args.output), 'wb') as out:
    if not args.slot0 == '':
      out.write(slot0data)
    elif not args.slot1 == '':
       out.write(slot1data)  
    out.close()
    assert getsize(join(args.directory, args.output)) <= REAL_FWSIZE
    print("Slot image (size: " + str(getsize(join(args.directory, args.output))) + ") available at: " + str(join(args.directory, args.output)))

if __name__ == "__main__":
  main()