#!/usr/bin/python3

import sys
import argparse
from struct import *

try:
    import xxhash
except ImportError as error:
        print("You don't have xxhash installed. pip3 install xxhash")

CONFIG_DATA_HEADER_TAG = 0x65494350 # "PCIe"
CONFIG_DATA_VERSION = 0x00000001

def load_binary_file(filename, size_alignment, max_size):
    try:
        file = open(filename, "rb")
    except:
        print("Failed to open file {0} for reading!".format(file))
        sys.exit(-1)

    try:
        data = file.read()
    except:
        print("Failed to read contents of file {0}!".format(file))
        sys.exit(-1)

    data_size = len(data)
    if 0 != (data_size % size_alignment):
        print("data file '{0}' size ({1}) is not a multiple of {2}!".format(filename, data_size, size_alignment))
        sys.exit(-1)
    if data_size > max_size:
        print("data file '{0}' size ({1}) exceeds {2}!".format(filename, data_size, max_size))
        sys.exit(-1)

    return data

def make_config_file(config_data_file, stage_1, stage_2, stage_3):
    if None == stage_1:
        stage_1_data_size = 0
    else:
        stage_1_data = load_binary_file(stage_1, 12, 3072)
        stage_1_data_size = len(stage_1_data)

    if None == stage_2:
        stage_2_firmware_size = 0
    else:
        stage_2_firmware = load_binary_file(stage_2, 4, 32768)
        stage_2_firmware_size = len(stage_2_firmware)

    if None == stage_3:
        stage_3_data_size = 0
    else:
        stage_3_data = load_binary_file(stage_3, 12, 3072)
        stage_3_data_size = len(stage_3_data)

    total_file_size = 32 + stage_1_data_size + stage_2_firmware_size + stage_3_data_size
    config_data = pack("<IIII", total_file_size, stage_1_data_size, stage_2_firmware_size, stage_3_data_size)

    x = xxhash.xxh64()
    x.update(config_data)
    if stage_1_data_size > 0:
        x.update(stage_1_data)
    if stage_2_firmware_size > 0:
        x.update(stage_2_firmware)
    if stage_3_data_size > 0:
        x.update(stage_3_data)

    config_header = pack("<IIQ", CONFIG_DATA_HEADER_TAG, CONFIG_DATA_VERSION, x.intdigest())

    file = open(config_data_file, "wb")
    file.write(config_header)
    file.write(config_data)
    if stage_1_data_size > 0:
        file.write(stage_1_data)
    if stage_2_firmware_size > 0:
        file.write(stage_2_firmware)
    if stage_3_data_size > 0:
        file.write(stage_3_data)
    file.close()

    print("Saved {0} bytes to PCIe config data file {1}.".format(total_file_size, config_data_file))

parser = argparse.ArgumentParser()
parser.add_argument("config_data_file")
parser.add_argument("-1", "--stage_1", help="stage_1_instructions_file")
parser.add_argument("-2", "--stage_2", help="stage_2_firmware_file")
parser.add_argument("-3", "--stage_3", help="stage_3_instructions_file")

args = parser.parse_args()

print("config_data_file:", args.config_data_file)
print("stage_1_instructions_file:", args.stage_1)
print("stage_2_firmware_file:", args.stage_2)
print("stage_3_instructions_file:", args.stage_3)

make_config_file(args.config_data_file, args.stage_1, args.stage_2, args.stage_3)
