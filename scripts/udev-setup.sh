#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "You need to be root/sudo in order to run this script." >&2
  exit 1
fi

set -o pipefail

OP=

display_help()
{
   echo "Manage udev rules for Intel(R) RealSenseID(TM)."
   echo
   echo "usage: udev-setup.sh [-h] [-i|-r]"
   echo
   echo "Options:"
   echo "  -i | --install       Install udev rules."
   echo "  -r | --remove        Remove/Uninstall udev rules."
   echo "  -h | --help          Show help message and exit."
   echo
}

arg_error()
{
    echo >&2
    echo "** Error: " "$@" >&2
    echo >&2
    display_help
    exit 1
}

set_op()
{
  if [ -z "${OP}" ]; then
    OP="$1"
  else
    arg_error "Can only specify install or uninstall option once to choose script operation mode!"
  fi
}

while [[ "$#" -gt 0 ]]; do
  case $1 in
    -r|--remove) set_op "uninstall"; ;;
    -i|--install) set_op "install"; ;;
    -h|--help) display_help; exit 0 ;;
    *) arg_error "Unknown option:" "$1" ;;
  esac
  shift
done

# For rule debugging:
#   udevadm info --name=/dev/video0
#   udevadm info --name=/dev/ttyACM0

UDEV_RULES_FILE_PATH=/etc/udev/rules.d/99-realsenseid-libusb.rules
UDEV_RULES=$(cat <<-END
##Version=1.0##

# Device rules for Intel F45x RealSenseID devices

# Allow non-root access to preview through libuvc
SUBSYSTEM=="usb",  ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="00dd", GROUP="plugdev", MODE="0664"
SUBSYSTEM=="usb",  ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="2aad", ATTRS{idProduct}=="6373", GROUP="plugdev", MODE="0664"

# Allow non-root access to F45x serial ports
SUBSYSTEM=="tty", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="00dd", GROUP="dialout", MODE="0664"
SUBSYSTEM=="tty", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="2aad", ATTRS{idProduct}=="6373", GROUP="dialout", MODE="0664"
END
)

case $OP in
  uninstall)
    echo "** Uninstalling udev rules to ${UDEV_RULES_FILE_PATH}"
    [ -f ${UDEV_RULES_FILE_PATH} ] && rm -f ${UDEV_RULES_FILE_PATH}
    ;;

  install)
    echo "** Installing udev rules ${UDEV_RULES_FILE_PATH}"
    echo "${UDEV_RULES}" > ${UDEV_RULES_FILE_PATH}
    ;;

  *)
    arg_error "** No option provided!"
    ;;
esac

echo "   * Reloading udev rules"
udevadm control --reload-rules

echo "   * Applying udev rules"
udevadm trigger

echo "** Done!"


