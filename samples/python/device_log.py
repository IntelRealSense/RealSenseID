"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.
"""

import rsid_py

PORT = 'COM4'

if __name__ == '__main__':
    with rsid_py.DeviceController(PORT) as d:
        log = d.fetch_log()
    with open("device.log", 'w') as f:
        f.write(log)
    print(f'Saved {len(log)} bytes to device.log')
