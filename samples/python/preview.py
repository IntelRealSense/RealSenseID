"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
"""
import time
import os
import rsid_py


def on_image(frame):
    print(f'got_frame #{frame.number} {frame.width}x{frame.height}')
    if frame.number > 10:
        os._exit(0)


if __name__ == '__main__':
    preview_cfg = rsid_py.PreviewConfig()
    preview_cfg.camera_number = 0
    p = rsid_py.Preview(preview_cfg)
    p.start(on_image)
    while True: time.sleep(10)