#!/usr/bin/env python3

import sys
from argparse import ArgumentParser
from enum import Enum
from struct import pack
import numpy as np
from itertools import chain

def array_zero_padding(a, n):
    if(a.size > n):
        raise ValueError("Exceeded max number of points")
    return np.pad(a, (0, n - a.size), mode='constant', constant_values=0)

# Persistant configuration data
manuf_name  = 'Esperanto'
part_num    = 0x11345663
serial_num  = 0x12345678
module_rev  = 0x2211ab44
form_factor = 'pcie'

# LUT of freq/volt pairs for mnn, sram, noc, pcl, ddr, mxn
numOfRails = 6
maxNumOfPoints = 11

mnn_freq = np.array([600,  700,  800]) # MHz
mnn_volt = np.array([0x32, 0x36, 0x3A]) # 500mV, 520mV, 540mV
mnn_freq  = array_zero_padding(mnn_freq, maxNumOfPoints)
mnn_volt =  array_zero_padding(mnn_volt, maxNumOfPoints)

sram_freq = np.array([600,  700,  800]) # MHz
sram_volt = np.array([0x64, 0x68, 0x6C]) # 750mV, 770mV, 790mV
sram_freq  = array_zero_padding(sram_freq, maxNumOfPoints)
sram_volt =  array_zero_padding(sram_volt, maxNumOfPoints)

noc_freq = np.array([400,  450,  500 ]) # MHz
noc_volt = np.array([0x2F, 0x32, 0x37]) # 485mV, 500mV, 525mV
noc_freq  = array_zero_padding(noc_freq, maxNumOfPoints)
noc_volt =  array_zero_padding(noc_volt, maxNumOfPoints)

pcl_freq = np.array([250,  500,  1000]) # MHz
pcl_volt = np.array([0x1C, 0x1C, 0x1C]) # 775mV, 775mV, 775mV
pcl_freq  = array_zero_padding(pcl_freq, maxNumOfPoints)
pcl_volt =  array_zero_padding(pcl_volt, maxNumOfPoints)

ddr_freq = np.array([933,  1066]) # MHz
ddr_volt = np.array([0x6E, 0x78]) # 800mV, 850mV
ddr_freq  = array_zero_padding(ddr_freq, maxNumOfPoints)
ddr_volt =  array_zero_padding(ddr_volt, maxNumOfPoints)

mxn_freq = np.array([600,  1000, 1500]) # MHz
mxn_volt = np.array([0x78, 0x78, 0x78]) # 850mV, 850mV, 850mV
mxn_freq  = array_zero_padding(mxn_freq, maxNumOfPoints)
mxn_volt =  array_zero_padding(mxn_volt, maxNumOfPoints)

vmin_lut = list(chain.from_iterable(zip(mnn_freq, mnn_volt, sram_freq, sram_volt, noc_freq, noc_volt, pcl_freq, pcl_volt, ddr_freq, ddr_volt, mxn_freq, mxn_volt)))

try:
    import xxhash
except ImportError as error:
    print("You don't have xxhash installed. pip3 install xxhash")
    sys.exit(1)

form_factors = ['none', 'pcie', 'dual_m2']
def auto_int(x): return int(x, 0)

parser = ArgumentParser(description='Create binary image with provided parameters')
parser.add_argument('-o', '--output', required=True, help='Output file path')
parser.add_argument('--tag',          type=auto_int, default=0x4d4f4944, help='Config data header tag (4 bytes)')
parser.add_argument('--version',      type=auto_int, required=True, help='Config data version (4 bytes)')
parser.add_argument('--fw_release_rev', type=auto_int, required=True, help='Firmware Release revision (4 bytes)')
parser.add_argument('--scp_size', type=auto_int, required=True, help='SCP size (2 bytes)')
parser.add_argument('--l2_size', type=auto_int, required=True, help='L2 cache size (2 bytes)')
parser.add_argument('--l3_size', type=auto_int, required=True, help='L3 cache size (2 bytes)')
parser.add_argument('--sp_pmic_interface_ver', type=auto_int, default=0x0000000, help='SP-PMIC interface version (4 bytes)')

args = parser.parse_args()

# Pack persistent config data
persistent_config_data = pack('<16sQIIB'+'HB'*(numOfRails*maxNumOfPoints),
    bytes(manuf_name,'ascii'),
    serial_num,
    module_rev,
    part_num,
    form_factors.index(form_factor),
    *(np.ravel(vmin_lut)))

# Pack variable config data
non_persistent_config = pack('<IHHHI',   
    args.fw_release_rev,
    args.scp_size,
    args.l2_size,
    args.l3_size,
    args.sp_pmic_interface_ver)

# Pack header (TAG + VERSION + XXHASH)
x = xxhash.xxh64(persistent_config_data + non_persistent_config).intdigest()
config_header = pack('<IIQ',
    args.tag,
    args.version,
    x)
    
with open(args.output, 'wb') as f:
    print(f'Writing {args.output}')
    print(f'  header ({len(config_header)} bytes)')
    print(f'  data   ({len(persistent_config_data) + len(non_persistent_config)} bytes)')
    f.write(config_header)
    f.write(persistent_config_data)
    f.write(non_persistent_config)

