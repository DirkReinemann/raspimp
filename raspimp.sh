#!/bin/bash

usage()
{
    echo "Rapimp helper script."
    echo
    echo "Usage: $0 [Options]"
    echo
    echo "Options:"
    printf "  %-20s %s\n" "-d" "deploy application to raspberry pi"
    printf "  %-20s %s\n" "-v" "start application on virtual display"
    exit 1
}

deploy()
{
    make
    zip -r9 raspimp.zip raspimp.c keyboard.c keyboard.h Makefile raspimp.sql raspimp.css raspimp.glade keyboard.glade play.png pause.png stop.png shutdown.png
    scp raspimp.zip raspberrypi:/tmp
    rm raspimp.zip

    ssh -t raspberrypi '
        cd /tmp
        unzip raspimp.zip
        rm raspimp.zip
        make
        sudo -v
        sudo mkdir -p /usr/share/raspimp
        sudo cp raspimp.sql /usr/share/raspimp/raspimp.sql
        sudo cp raspimp.glade /usr/share/raspimp/raspimp.glade
        sudo cp raspimp.css /usr/share/raspimp/raspimp.css
        sudo cp keyboard.glade /usr/share/raspimp/keyboard.glade
        sudo cp play.png /usr/share/raspimp/play.png
        sudo cp pause.png /usr/share/raspimp/pause.png
        sudo cp stop.png /usr/share/raspimp/stop.png
        sudo cp shutdown.png /usr/share/raspimp/shutdown.png
        sudo killall raspimp ; sudo cp raspimp /usr/bin/raspimp
        rm Makefile raspimp raspimp.c keyboard.c keyboard.h raspimp.sql raspimp.css raspimp.glade keyboard.glade play.png pause.png stop.png shutdown.png
    '
}

virtualdisplay()
{
    local savedisplay="$DISPLAY"

    make
    nohup Xephyr -br -ac -noreset -softCursor -screen 800x480 :11 > xephyr.out 2>&1 &
    export DISPLAY=':11'
    nohup ./raspimp > raspimp.out 2>&1 &
    echo "Press Enter to close ..."
    read
    killall Xephyr
    killall raspimp
    export DISPLAY="$savedisplay"
}

if [ $# == 0 ]; then
    usage
fi

while getopts "dsbv" opt; do
    case $opt in
        d)
            deploy
        ;;
        v)
            virtualdisplay
        ;;
        \?)
            usage
        ;;
        :)
            usage
        ;;
    esac
done

exit 0
