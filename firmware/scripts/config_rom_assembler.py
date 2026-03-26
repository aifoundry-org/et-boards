#!/usr/bin/python3

#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------


import argparse
from array import *
import ctypes
import yaml

def lookup_reg(reg_data, reg_name):
    #Look up register offset
    found = False
    for reg_info in reg_data['registers']:
        if reg_info['name'] == reg_name:
            offset = reg_info['offset']
            mem_space_str = reg_info['mem_space']
            found = True
            break
    if found == False:
        raise AttributeError("Invalid reg name", name)

    #Look up mem space
    found = False
    for mem_space_data in reg_data['memSpaces']:
        if mem_space_data['name'] == mem_space_str:
            mem_space = mem_space_data['memSpace']
            found = True
            break
    if found == False:
        raise AttributeError("Invalid memspace", mem_space_str)

    return mem_space, offset

def parse_file(op_data, reg_data, outfile, verbose):
    out_array = bytearray()

    for op in op_data['ops']:
        if verbose: print(op)

        if op['op'] == 'w':
            opCode = 1
            memSpace, offset = lookup_reg(reg_data, op['reg'])

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['val']
            dw2 = op['mask'] if 'mask' in op else 0xFFFFFFFF
        elif op['op'] == 'rmw':
            opCode = 2
            memSpace, offset = lookup_reg(reg_data, op['reg'])

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['val']
            dw2 = op['mask']
        elif op['op'] == 'poll':
            opCode = 3
            memSpace, offset = lookup_reg(reg_data, op['reg'])

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['val']
            dw2 = op['mask']
        elif op['op'] == 'wait':
            opCode = 4
            dw0 = opCode << 28
            dw1 = op['tickCount']
            dw2 = 0
        elif op['op'] == 'infinite_poll':
            opCode = 5
            memSpace, offset = lookup_reg(reg_data, op['reg'])

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['val']
            dw2 = op['mask']
        else:
            raise AttributeError("Invalid op", op['op'])

        if verbose: print("Out: 0x%08x 0x%08x 0x%08x" % (dw0, dw1, dw2))

        out_array += dw0.to_bytes(4, byteorder='little', signed=False)
        out_array += dw1.to_bytes(4, byteorder='little', signed=False)
        out_array += dw2.to_bytes(4, byteorder='little', signed=False)

    #Insert terminator op
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)

    with open(outfile, 'wb') as f:
        f.write(out_array)
    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--opfile",
                        required=True,
                        help='Path to file containing ops')
    parser.add_argument("--regfile",
                        required=True,
                        help='Path to file containing register definitions')
    parser.add_argument("--outfile",
                        required=True,
                        help='Output path of instructions after phy is loaded')
    parser.add_argument("--verbose",
                        required=False,
                        default=False,
                        help='Trace operation')
    args = parser.parse_args()

    # Read YAML file
    with open(args.opfile, 'r') as stream:
        op_data = yaml.safe_load(stream)

    with open(args.regfile, 'r') as stream:
        reg_data = yaml.safe_load(stream)

    parse_file(op_data, reg_data, args.outfile, args.verbose)
