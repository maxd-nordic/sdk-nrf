#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import sys
import usb.core
import usb.util
import argparse
import logging as default_logging
import serial
import serial.tools.list_ports
import os
import time
from west.commands import WestCommand
import west.log
from tempfile import TemporaryDirectory
from zipfile import ZipFile
import re

RECOVER_NRF53 = b"\x8e"  # connectivity_bridge bootloader mode
RECOVER_NRF91 = b"\x8f"  # nrf91 bootloader mode
RESET_NRF53 = b"\x90"  # reset nrf53
RESET_NRF91 = b"\x91"  # reset nrf91


def add_args_to_parser(parser, default_chip=""):
    # define argument to decide whether to recover nrf53 or nrf91
    parser.add_argument(
        "--chip",
        type=str,
        help="bootloader mode to trigger: nrf53 or nrf91",
        default=default_chip,
    )
    parser.add_argument("--vid", type=int, help="vendor id", default=0x1915)
    parser.add_argument("--pid", type=int, help="product id", default=0x910A)
    parser.add_argument("--serial", type=str, help="serial number", default=None)
    parser.add_argument(
        "--debug", action="store_true", help="enable debug logging", default=False
    )


def find_bulk_interface(device, descriptor_string, logging):
    for cfg in device:
        for intf in cfg:
            # Attempt to detach kernel driver (only necessary on Linux)
            if sys.platform == "linux" and device.is_kernel_driver_active(
                intf.bInterfaceNumber
            ):
                try:
                    device.detach_kernel_driver(intf.bInterfaceNumber)
                except usb.core.USBError as e:
                    logging.error(f"Could not detach kernel driver: {str(e)}")
            if usb.util.get_string(device, intf.iInterface) == descriptor_string:
                return intf
    return None


def trigger_bootloader(vid, pid, chip, logging, reset_only, serial=None):
    if serial is not None:
        device = usb.core.find(idVendor=vid, idProduct=pid, serial_number=serial)
        if device is None:
            logging.error(f"Device with serial number {serial} not found")
            return
        serial_number = serial
    else:
        # Print serial numbers if multiple devices are found
        devices = list(usb.core.find(find_all=True, idVendor=vid, idProduct=pid))
        if len(devices) == 0:
            logging.error("No devices found.")
            return
        if len(devices) > 1:
            logging.info("Multiple devices found.")
            for device in devices:
                serial_number = usb.util.get_string(device, device.iSerialNumber)
                logging.info(f"Serial Number: {serial_number}")
            logging.warning(
                "Please specify the serial number with the --serial option."
            )
            return
        else:
            device = devices[0]
            serial_number = usb.util.get_string(device, device.iSerialNumber)

    # Check if device description is MCUBOOT
    if "MCUBOOT" in device.product:
        logging.warning("Device is already in bootloader mode")
        if chip != "nrf53":
            logging.error(
                "The device is in nRF53 bootloader mode, " + \
                "but you are trying to flash an nRF91 image. " + \
                "Please program the connectivity_bridge firmware first."
            )
            return
        return serial_number

    # Find the bulk interface
    bulk_interface = find_bulk_interface(device, "CMSIS-DAP v2", logging)
    if bulk_interface is None:
        logging.error("Bulk interface not found")
        return

    # Find the out endpoint
    out_endpoint = None
    for endpoint in bulk_interface:
        if (usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_OUT):
            out_endpoint = endpoint
            break
    if out_endpoint is None:
        logging.error("OUT endpoint not found")
        return

    # Find the in endpoint
    in_endpoint = None
    for endpoint in bulk_interface:
        if (usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_IN):
            in_endpoint = endpoint
            break
    if in_endpoint is None:
        logging.error("IN endpoint not found")
        return

    if reset_only:
        if chip == "nrf53":
            logging.info("Trying to reset nRF53...")
            try:
                out_endpoint.write(RESET_NRF53)
                logging.debug("Data sent successfully.")
            except usb.core.USBError as e:
                logging.error(f"Failed to send data: {e}")
                return
        else:
            logging.info("Trying to reset nRF91...")
            try:
                out_endpoint.write(RESET_NRF91)
                logging.debug("Data sent successfully.")
            except usb.core.USBError as e:
                logging.error(f"Failed to send data: {e}")
                return
            # Read the response
            try:
                data = in_endpoint.read(in_endpoint.wMaxPacketSize)
                logging.debug(f"Response: {data}")
            except usb.core.USBError as e:
                logging.error(f"Failed to read data: {e}")
        return serial_number

    # Send the command to trigger the bootloader
    if chip == "nrf53":
        logging.info("Trying to put nRF53 in bootloader mode...")
        try:
            out_endpoint.write(RECOVER_NRF53)
            logging.debug("Data sent successfully.")
        except usb.core.USBError as e:
            logging.error(f"Failed to send data: {e}")
            return
        # Device should be in bootloader mode now, no need to read the response
    else:
        logging.info("Trying to put nRF91 in bootloader mode...")
        try:
            out_endpoint.write(RECOVER_NRF91)
            logging.debug("Data sent successfully.")
        except usb.core.USBError as e:
            logging.error(f"Failed to send data: {e}")
            return
        # Read the response
        try:
            data = in_endpoint.read(in_endpoint.wMaxPacketSize)
            logging.debug(f"Response: {data}")
        except usb.core.USBError as e:
            logging.error(f"Failed to read data: {e}")
    return serial_number


def perform_dfu(serial_number, image, chip, logging):
    # extract the hexadecimal part of the serial number
    match = re.search(r"(THINGY91X_)?([A-F0-9]+)", serial_number)
    if match is None:
        logging.error("Serial number not found")
        sys.exit(1)
    serial_number_digits = match.group(2)

    # Find the corresponding serial port
    for _ in range(10):
        port_info = None
        for port_info in serial.tools.list_ports.comports():
            if port_info.serial_number is None:
                continue
            logging.debug(
                f"Serial port: {port_info.device} has serial number: {port_info.serial_number}"
            )
            if serial_number_digits in port_info.serial_number:
                logging.debug(f"Serial port: {port_info.device}")
                serial_number = port_info.serial_number
                break
        if port_info is None:
            logging.error("Serial port not found")
            sys.exit(1)
        if chip == "nrf53":
            if "MCUBOOT" in port_info.description:
                break
        else:
            break
        logging.debug("Waiting for device to enumerate...")
        time.sleep(1)

    # Assemble DFU update command:
    command = (
        "nrfutil device x-execute -o program --args "
        + f"'firmware[file]={image}' "
        + f"--device-filter 'serialNumber={serial_number}' "
        + "--override-traits mcuBoot --inject 'mcuBoot[vcom]=0'"
    )

    logging.debug(f"Executing command: {command}")
    os.system(command)


def detect_family_from_zip(zip_file, logging):
    with TemporaryDirectory() as tmpdir:
        with ZipFile(zip_file, "r") as zip_ref:
            zip_ref.extractall(tmpdir)
        files = os.listdir(tmpdir)
        for f in files:
            if "manifest.json" in f:
                with open(os.path.join(tmpdir, f)) as manifest:
                    manifest_content = manifest.read()
                    if "nrf53" in manifest_content:
                        return "nrf53"
                    elif "nrf91" in manifest_content:
                        return "nrf91"
    return None


def main(args, reset_only, logging=default_logging):
    # if logging does not containt .error function, map it to .err
    if not hasattr(logging, "error"):
        logging.debug = logging.dbg
        logging.info = logging.inf
        logging.warning = logging.wrn
        logging.error = logging.err

    # determine chip family
    chip = args.chip
    if hasattr(args, 'image') and args.image is not None:
        # if image is provided, try to determine chip family from image
        chip = detect_family_from_zip(args.image, logging)
        if chip is None:
            logging.error("Could not determine chip family from image")
            sys.exit(1)
    if len(args.chip) > 0 and args.chip != chip:
        logging.error("Chip family does not match image")
        sys.exit(1)
    if chip not in ["nrf53", "nrf91"]:
        logging.error("Invalid chip")
        sys.exit(1)

    serial_number = trigger_bootloader(args.vid, args.pid, chip, logging, reset_only, args.serial)
    if serial_number is not None:
        if reset_only:
            logging.info(f"{chip} on {serial_number} has been reset")
        else:
            logging.info(f"{chip} on {serial_number} is now in bootloader mode")
    else:
        sys.exit(1)

    # Only continue if an image file is provided
    if hasattr(args, 'image') and args.image is not None:
        perform_dfu(serial_number, args.image, chip, logging)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Thingy91X DFU", allow_abbrev=False)
    parser.add_argument("--image", type=str, help="application update file")
    add_args_to_parser(parser)
    args = parser.parse_args()

    default_logging.basicConfig(
        level=default_logging.DEBUG if args.debug else default_logging.INFO
    )

    main(args, reset_only=False, logging=default_logging)


class Thingy91XDFU(WestCommand):
    def __init__(self):
        super(Thingy91XDFU, self).__init__(
            "thingy91x-dfu",
            "Thingy:91 X DFU",
            "Put Thingy:91 X in DFU mode and update using MCUBoot serial recovery.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        add_args_to_parser(parser)
        parser.add_argument("image", type=str, help="application update file")

        return parser

    def do_run(self, args, unknown_args):
        main(args, reset_only=False, logging=west.log)

class Thingy91XReset(WestCommand):
    def __init__(self):
        super(Thingy91XReset, self).__init__(
            "thingy91x-reset",
            "Thingy:91 X Reset",
            "Reset Thingy:91 X.",
        )
    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        add_args_to_parser(parser, default_chip="nrf91")

        return parser

    def do_run(self, args, unknown_args):
        main(args, reset_only=True, logging=west.log)
