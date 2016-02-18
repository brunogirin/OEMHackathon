# How to install the OpenTRV concentrator code on a Raspberry Pi

## Requirements

You will need:

* A Raspberry Pi
* A SD card with [Raspbian](https://www.raspberrypi.org/downloads/raspbian/)
  installed
* An OpenTRV node with stats hub code (REV2 is good)
* A FTDI cable to connect the OpenTRV node to the Rasberry Pi

## Install dependencies

The OpenTRV concentrator is written in Java and requires the RXTX and Simple
JSON libraries to run. You can install them through `apt-get`:

    sudo apt-get install librxtx-java libjson-simple-java

## Create the folder structure

You can deploy the different files the way you prefer. The rest of this
document assumes you have the following folder and file structure:

    .
    +-- bin
    |   +-- localOpenTRVMonitor.sh
    +-- opentrv
        +-- config.json
        +-- lib
        |   +-- OpenTRV-comms-0.2.2.jar
        +-- .private
            +-- stats

## Build and install the JAR file

Clone the OpenTRV repository from GitHub:

    git clone git@github.com:DamonHD/OpenTRV.git

Go to the top level and bulid the code:

    cd OpenTRV
    ant

This will build a JAR file called `OpenTRV-comms-0.2.2.jar` in the `out_D`
folder. Copy this file to the `opentrv/lib` folder on the Pi.

## Identify the FTDI serial cable by ID

Plug in the FTDI cable into the OpenTRV device, making sure that the black
ground pin is closest to the corner of the box. Then plug the USB plug of the
cable to the Pi. Identify the ID of the cable:

    ls /dev/serial/by-id

It should show a single file with a name that looks like:

    usb-FTDI_TTL232R_FTGCVTIF-if00-port0

## Deploy a basic configuration file

Create a file called `config.json` in the `opentrv` folder on the Pi with the
following content, replacing the `serialPort` value with the full path of
your FTDI cable:

    {
        "serialPort": "/dev/serial/by-id/usb-FTDI_TTL232R_FTGCVTIF-if00-port0",
        "handlers": [
            {
                "name": "File log",
                "type": "uk.org.opentrv.comms.statshandlers.builtin.SimpleFileLoggingStatsHandler",
                "options": {
                    "statsDirName": "/home/pi/opentrv/.private/stats/"
                }
            }
        ]
    }

## Deploy an executable shell script

Create a file called `localOpenTRVMonitor.sh` in the `bin` folder with the
following content:

    #!/bin/sh

    JAVA_HOME=/usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt/
    export JAVA_HOME

    # OpenTRV comms lib version
    OPENTRVVER=0.2.2

    OPENTRVDIR=/home/pi/opentrv/
    OPENTRVJLDIR=$OPENTRVDIR/lib
    # From librxtx-java package...
    OPENTRVSTPJLDIR=/usr/share/java
    OPENTRVTPNLDIR=/usr/lib/jni

    exec $JAVA_HOME/bin/java -Xms2m -Xmx4m \
        -Djava.library.path=$OPENTRVTPNLDIR \
        -classpath "$OPENTRVJLDIR/OpenTRV-comms-${OPENTRVVER}.jar:$OPENTRVSTPJLDIR/*" \
        uk.org.opentrv.comms.util.EventDrivenV0p2CLIFollower \
        $OPENTRVDIR/config.json

Make it executable:

    chmod +x bin/localOpenTRVMonitor.sh

Run it!

    bin/localOpenTRVMonitor.sh
