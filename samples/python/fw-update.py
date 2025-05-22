#!/usr/bin/env python3

"""
License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.
"""

import argparse
import pathlib
import threading
from argparse import SUPPRESS, ArgumentParser

import rsid_py

try:
    from tqdm import tqdm
    tqdm_installed = False
except ImportError:
    print('tqdm not installed - progress bar will not be available. pip install tqdm to enable.')
    tqdm_installed = False


def build_arg_parser():
    class MultiFormatter(argparse.RawTextHelpFormatter,
                         argparse.ArgumentDefaultsHelpFormatter):
        pass

    arg_parser = ArgumentParser(prog='fw-update-py', add_help=False,
                                formatter_class=MultiFormatter)
    options = arg_parser.add_argument_group('Options')
    options.add_argument('-h', '--help', action='help', default=SUPPRESS,
                         help='Show this help message and exit.')
    options.add_argument("-p", "--port", help="Device port", required=True, type=str)
    options.add_argument("-f", "--file", help="Firmware binary file path", required=True, type=pathlib.Path)
    options.add_argument("--dry-run", help="Show summary report and exit", required=False, default=False,
                         action='store_true')
    options.add_argument("--skip-online", help="Skip checking for latest version online",
                         required=False, default=False,
                         action='store_true')

    options.add_argument("--force-version", help="Force update even if host version mismatch",
                         action='store_true',
                         default=False)

    options.add_argument("--force-full", help="Force update of modules even if they already exist \n"
                                              "in the current device \nfirmware. This will update all modules. \n",
                         action='store_true')

    return arg_parser


report_template = """
Summary Report

* Device
────────
    * Type: {device_type}
    * Serial number: {serial_number}
    * Serial port: {serial_port}
    * Firmware version: {firmware_version}
    * Recognition module version: {recognition_module_version}

* Firmware File
───────────────
    * Firmware file path: {bin_file_path}
    * Firmware version: {bin_firmware_version}
    * Recognition module version: {bin_recognition_module_version}

* Compatibility Matrix
──────────────────────               
              * Host:   {host_compat}
                        {host_msg}
"""

update_remote_template = """
Update Checker Report: {update_note}

* Current Software
──────────────────
 You are currently running
          * Firmware: {local_fw_version}
          * SDK/Host: {local_sw_version}

* Remote Software
──────────────────
 The following software is available online:
          * Firmware: {remote_fw_version}
          * SDK/Host: {remote_sw_version}
      * Download URL: {release_url}
 * Release notes URL: {release_notes_url}

"""

COMPATIBLE = "✓ Compatible"
NOT_COMPATIBLE = "𐄂 Not Compatible"
RUNNING_LATEST = "✓ You are running the latest software"
NEWER_AVAILABLE = "𐄂 Newer software is available online"

if __name__ == '__main__':
    args = build_arg_parser().parse_args()

    update_progress: float = 0
    pbar = None

    if not args.skip_online:
        update_available, local_info, remote_info = rsid_py.UpdateChecker.is_update_available(args.port)

        print(update_remote_template.format(local_fw_version=local_info.fw_version_str,
                                            local_sw_version=local_info.sw_version_str,
                                            remote_fw_version=remote_info.fw_version_str,
                                            remote_sw_version=remote_info.sw_version_str,
                                            release_url=remote_info.release_url,
                                            release_notes_url=remote_info.release_notes_url,
                                            update_note=NEWER_AVAILABLE if update_available else RUNNING_LATEST))

    def progress_callback(progress: float) -> None:
        global update_progress
        update_progress = progress
        percent = int(100 * progress)
        if pbar is not None:
            pbar.update(percent)
        else:
            print(f"Progress: {percent}%")


    with rsid_py.FWUpdater(str(args.file), args.port) as updater:
        fw_file_info = updater.get_firmware_bin_info()
        device_fw_info = updater.get_device_firmware_info()        
        host_compat, host_msg = updater.is_host_compatible(device_fw_info.device_type)

        print(report_template.format(
            device_type = device_fw_info.device_type,
            serial_number=device_fw_info.serial_number,
            serial_port=args.port,
            firmware_version=device_fw_info.fw_version,
            bin_file_path=args.file,
            recognition_module_version=device_fw_info.recognition_version,
            bin_firmware_version=fw_file_info.fw_version,
            bin_recognition_module_version=fw_file_info.recognition_version,
            host_compat=COMPATIBLE if host_compat else NOT_COMPATIBLE, host_msg=host_msg))

        if args.dry_run:
            exit(0)

        print("Started update process...")
        if tqdm_installed:
            pbar = tqdm(total=100, desc="Updating firmware")

        status = updater.update(force_version=args.force_version,
                                force_full=args.force_full,
                                progress_callback=progress_callback)

        if status == rsid_py.Status.Ok:
            condition = threading.Condition()
            with condition:
                condition.wait_for(lambda: update_progress == 1)
            print("Done!")
        else:
            print(f"Failed to start update with status {status}")
            exit(status.value)
