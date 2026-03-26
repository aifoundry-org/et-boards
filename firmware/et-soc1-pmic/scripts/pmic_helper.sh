#!/bin/bash

set -e

# initialize the environment
eap_dir=/usr/local/eap
default_image=/home/et/firmware_images/release/pmic_images/latest
source $eap_dir/eap.env ""
supported_actions=(update smoke_test restore reboot)

script_name=$(basename $0)
# Defaults
declare -a selected_actions
eap_device=
iteration=1
output=
output_default=$(mktemp)
pmic_image=
timeout=5minutes

function usage()
{
    cat << HEREDOC

    Usage: $script_name --action ACTION --eap_device DEVICE [--pmic_image PATH] [--iteration COUNT]
                        [--output PATH] [--timeout TIMEOUT] [--help]

    arguments:
      -a, --action ACTION       pass in ACTION to be performed. This argument can be used multiple times to
                                perform more than one action. If multiple actions are requires then they will
                                be performed in sequence {${supported_actions[@]}}
                                See the details of each action as following:
        update                  update PMIC firmware to provided pmic_image
                                NOTE: This leaves the silicon cards in stale state (requires reboot)
          -p, --pmic_image PATH the PATH to the PMIC image
        smoke_test              run smoke tests
                                NOTE: This leaves the silicon cards in stale state (requires reboot)
          -i, --iteration COUNT pass in the test iteration COUNT (default: $iteration)
          -o, --output PATH     pass in PATH to the output file to save the UART logs (default: stderr)
          -t, --timeout TIMEOUT pass in TIMEOUT in seconds, minutes or hours (e.g.: $timeout)
        restore                 restore PMIC firmware to the default image ($default_image)
                                NOTE: This leaves the silicon cards in stale state (requires reboot)
        reboot                  reboot the silicon host machine
      -e, --eap_device DEVICE   the EAP device name of silicon host machine (e.g. mv-swpcie23-dev0)
      --help                    show this help message and exit

HEREDOC
}

# use getopt and store the output into $opts
opts=$(getopt -o "a:e:i:o:p:t:h" --long "action:,eap_device:,iteration:,output:,pmic_image:,timeout:,help" -n "$script_name" -- "$@")
eval set -- "$opts"

while true; do
  case "$1" in
    -a | --action ) selected_actions+=($(echo "${supported_actions[@]}" | grep -ow "$2")) ; shift 2 ;;
    -e | --eap_device ) if eap_list | grep -qw "$2"; then eap_device=$2; else echo "Invalid eap_device: $2"; exit 1; fi ; shift 2 ;;
    -i | --iteration ) if echo "$2" | grep -qP '\d+'; then iteration=$2; else echo "Invalid iteration: $2"; exit 1; fi ; shift 2 ;;
    -o | --output ) output=$(realpath $2); shift 2 ;;
    -p | --pmic_image ) pmic_image=$(realpath $2); shift 2 ;;
    -t | --timeout ) if echo "$2" | grep -Pq '\d+(?:second|minute|hour)s?'; then timeout=$2; else echo "Invalid timeout: $2"; exit 1; fi ; shift 2 ;;
    --help ) usage; exit; ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

function usb_reset()
{
    if ! [[ $1 =~ ttyUSB[0-9]+ ]]; then
        echo "Argument must be a USB serial tty device: example ttyUSB0"
        return 1
    fi
    local devname=$(basename $1)
    local rootpath="/sys/bus/usb-serial/devices"
    local devpath=$rootpath/$devname
    local fullpath=$(readlink -f $devpath)
    if ! [ -e "$fullpath" ]; then
        echo "Unable to find USB serial tty device $devname!"
        return 1
    fi
    echo "Attempting to reset $devname"
    local treepath=$fullpath
    local usbdevice
    local midpath
    local underpath
    local lowestdriver
    # Control files
    local fileon="/sys/bus/usb/drivers/usb/bind"
    local fileoff="/sys/bus/usb/drivers/usb/unbind"
    while true; do
        # Chop off path components as we work our way up the USB device tree
        if [[ $treepath =~ ^(.+)\/([^\/]+)$ ]]; then
            underpath=$midpath
            midpath=$treepath
            treepath=${BASH_REMATCH[1]}
            usbdevice=${BASH_REMATCH[2]}
        else
            echo "Fell out of device tree!"
            return 1
        fi
        # Stop search at the root of the device tree
        if [[ $usbdevice = devices ]]; then
            echo "Searched entire device tree but came up empty!"
            return 1
        fi
        echo "Searching $usbdevice"
        # Device must have a driver
        local driverfile="$midpath/driver"
        fullpath=$(readlink -f $driverfile)
        if ! [ -e "$fullpath" ]; then
            echo "Skipping, device does not have a driver"
            continue
        fi
        # Driver path must end in bus/usb/drivers/usb
        if ! [[ $fullpath =~ bus[\/]usb[\/]drivers[\/]usb$ ]]; then
            echo "Skipping, device is not a USB hub"
            continue
        fi
        # At this point, we should have a saved path
        if [ -z "$underpath" ]; then
            echo "Lost track of device tree branches!"
            return 1
        fi
        # Device immediately underneath this device must have a driver, so we can detect its restoration
        local underdriver="$underpath/driver"
        fullpath=$(readlink -f $underdriver)
        if ! [ -e "$fullpath" ]; then
            echo "Skipping, device below this hub does not have a driver"
            continue
        fi
        # Remember this path, so we can validate, even if we have to reset hubs further upstream
        if [ -z "$lowestdriver" ]; then
            lowestdriver=$underdriver
        fi
        echo "Commanding $usbdevice OFF"
        # Tell kernel to unbind this device from the driver (should power it off if this hub is capable)
        if ! bash -c "echo $usbdevice > $fileoff"; then
            echo "Unable to write to $fileoff! Do you have root permissions?"
            return 1
        fi
        sleep 0.5
        # Wait for the device underneath to disappear, it should if we turned it off OK
        local isgone
        for i in {1..5}; do
            if ! stat $underpath &> /dev/null; then
                isgone=1
                break
            fi
            echo "Waiting $i"
            sleep 0.25
        done
        # Failed to turn the device off, advance to the next hub, maybe that hub is built better
        if [ -z "$isgone" ]; then
            echo "It did not turn off, trying next hub upstream"
            continue
        fi
        echo "Commanding $usbdevice ON"
        if ! bash -c "echo $usbdevice > $fileon"; then
            echo "Unable to write to $fileon! Do you have root permissions?"
            return 1
        fi
        sleep 0.5
        # Wait for the device to reappear and its driver to reregister, allow extra time
        # Can't just check for the ttyUSB entry to reappear, because its number might have changed
        # Can't just check for the device path to reappear, because its driver might still be wedged and fail to initialize
        local isback
        for i in {1..10}; do
            fullpath=$(readlink -f $underdriver)
            if [ -e "$fullpath" ]; then
                isback=1
                break
            fi
            echo "Waiting $i"
            sleep 0.25
        done
        # Failed to turn the device back on, maybe it is really stuck, also try next hub upstream
        if [ -z "$isgone" ]; then
            echo "It did not turn back on, trying next hub upstream"
            continue
        fi
        # If we made it this far, we're nearly done
        break
    done
    if [ -z "$lowestdriver" ]; then
        echo "Something went wrong while trying to search for driver!"
        return 1
    fi
    echo "Validating"
    # Allow time to wait for entire device tree to repopulate to what it was before
    local isrestored
    for i in {1..20}; do
        fullpath=$(readlink -f $underdriver)
        if [ -e "$fullpath" ]; then
            isrestored=1
            break
        fi
        echo "Waiting $i"
        sleep 0.25
    done
    if [ -z "$isrestored" ]; then
        echo "The driver was not restored!"
        return 1
    fi
    # The ttyUSB device should now be once again ready to use, although its number might have changed
    echo "Done!"
    return 0
}

function stop_uart_logging()
{
    local uart_pid=$1
    # kill uart
    kill -9 $uart_pid || true &> /dev/null
    wait $uart_pid || true &> /dev/null
    if [ "$output" = "$output_default" ]; then
        {
            echo "##########################################################################################"
            echo "PMIC UART Output"
            cat $output
            echo "##########################################################################################"
        } >&2
    fi
    rm $output_default
}

if [ "$(whoami)" != "root" ]; then
    echo "Please run this script as root!"
    exit 1
fi
endtime=$(date -ud "$timeout" +%s)

# Check that required options were provided
if [ -z "$eap_device" -o -z "${selected_actions}" ]; then
    echo "Missing required arguments: eap_device or or action!"
    usage
    exit 1
fi

hostname=$(echo ${eap_device} | grep -Po 'mv-swpcie[0-9]+' || true)
if [ -z "$hostname" ]; then
    echo "Failed to deduce hostname from $eap_device"
    exit 1
fi

if [ -z "$output" ]; then
    output=$output_default
fi

# Check if host machine of provided EAP device has an EAP bub device also.
# Hosts containing any bub device requires power cycle through EAP bub device
eap_pc_device=$(eap_list | grep $hostname | grep bub || echo $eap_device)

eap_init $eap_device

failure=
if printf '%s\0' "${selected_actions[@]}" | grep -qFxz -- 'update'; then
    # Flash PMIC firmware
    echo "Flashing PMIC firmware on $EAP_TARGET"
    (cd $eap_dir && updatepmicfw $pmic_image && sleep 3)
    echo "Turning PMIC off and on"
    (pmic off && sleep 1 && pmic on && sleep 1)
fi

if printf '%s\0' "${selected_actions[@]}" | grep -qFxz -- 'smoke_test'; then
    # Free up UART device if it is being used and start logging to UART
    uart_dev=$(get_ttyUSB $EAP_USB_PORT 0)
    fuser -k $uart_dev &> /dev/null || true
    usb_reset $uart_dev
    stty -F $uart_dev 115200 raw -echo -echok
    cat $uart_dev > $output &
    uart_pid=$!
    sleep 5
    # Check if PMIC UART is responding to commands
    echo -n "Checking if PMIC UART is responding to COMMANDs ... "
    responding=
    for i in {1..5}; do
        echo -e -n "x\r" > $uart_dev
        sleep 1
        if grep -q 'Unknown Command: x' $output; then
            echo "done"
            responding=1
            break
        fi
    done
    if [ -z "$responding" ]; then
        echo "failed, not responding. Can't run smoke tests!"
         let "failure+=1"
    fi
    # Run the tests
    once_flag=
    while [ -z "$failure" ]; do
        if [ -z "$once_flag" ]; then
            echo -n "Running PMIC FW smoke tests for $EAP_TARGET ... "
            echo -e -n "smoke-test $iteration\r" > $uart_dev
            once_flag=1
	fi
        if grep -q 'TEST END' $output; then
            echo "done"
            break
        elif grep -q 'TEST FAILED' $output; then
            echo "failed"
            let "failure+=1"
        elif [ "$(date -u +%s)" -gt "$endtime" ]; then
            echo "timed out"
            let "failure+=1"
        fi
        sleep 1
    done
    # This helps complete the hot-reset from host-machine
    echo -e -n "rsoc\r" > $uart_dev
    stop_uart_logging $uart_pid
fi

set +e

if printf '%s\0' "${selected_actions[@]}" | grep -qFxz -- 'restore'; then
    # Restore default PMIC firmware
    echo "Restoring default PMIC firmware on $EAP_TARGET"
    (cd $eap_dir && updatepmicfw $default_image)
fi

if printf '%s\0' "${selected_actions[@]}" | grep -qFxz -- 'reboot'; then
    rebooted=
    while [ "$(date -u +%s)" -le "$endtime" -a -z "$rebooted" ]; do
        # host_power_cycle normally takes 40-50 seconds, reserving
        # twice the time for the device to be up after power cycle
        expected_uptime=$(date -ud "2minute" +%s)
        if [ "$expected_uptime" -gt "$endtime" ]; then
            expected_uptime=$endtime
        fi
        (eap_init $eap_pc_device && host_power_cycle)
        while [ "$(date -u +%s)" -le "$expected_uptime" -a -z "$rebooted" ]; do
            if ping -c1 $hostname > /dev/null; then
                rebooted=1
            fi
        done
    done
    if [ -z "$rebooted" ]; then
        echo "Failed to reboot ${eap_device}"
        let "failure+=1"
    fi
fi

exit $failure
