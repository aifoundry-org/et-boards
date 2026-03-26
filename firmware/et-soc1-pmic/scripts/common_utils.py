#!/usr/bin/env python3

import ctypes, struct
from struct import unpack_from

# IMPORTANT: Must match the structure rtMetadata_t defined in config.h
class image_metadata(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("start_addr",               ctypes.c_uint32),
        ("rt_fw_version",            ctypes.c_uint32),
        ("supported_board_types",    ctypes.c_uint32),
        ("hash",                     ctypes.c_char * 16),
        ("checksum",                 ctypes.c_uint32),
        ("image_size",               ctypes.c_uint32),
        ("bl_fw_version",            ctypes.c_uint32),
        ("sp_comm_protocol_version", ctypes.c_uint32),
        ("metadata_version",         ctypes.c_uint32),
        ("build_type",               ctypes.c_uint32),
    ]

def calculateAdjustedChecksum32b(data, startOffset, endOffset):
    checksum = 0
    for i in range(startOffset, endOffset, 4):
        checksum += struct.unpack_from('<L', data, i)[0]
    return ((0 - checksum) & ((1 << 32) - 1))

def addMetadata(fwImage, metadataOffset):

  # Insert image size and checksum
  metadata = image_metadata.from_buffer(fwImage, metadataOffset)
  metadata.image_size = len(fwImage)
  assert (metadata.image_size > 0) and (metadata.checksum == 0)
  metadata.checksum = calculateAdjustedChecksum32b(fwImage, 0, len(fwImage))

  print("Metadata added to the image.")
  print("Image size: " + str(metadata.image_size))
  print("Image checksum: " + str(hex(metadata.checksum)))

  return fwImage
