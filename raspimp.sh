#!/bin/bash

usage()
{
    echo "Rapimp helper script."
    echo
    echo "Usage: $0 [Options]"
    echo
    echo "Options:"
    printf "  %-20s %s\n" "-d" "deploy application to raspberry pi"
    printf "  %-20s %s\n" "-s" "create raspimp.db from raspimp.sql"
    printf "  %-20s %s\n" "-b" "deploy raspimp.db to raspberry pi"
    printf "  %-20s %s\n" "-v" "start application on virtual display"
    exit 1
}

deploy()
{
    make
    zip -r9 raspimp.zip raspimp.c keyboard.c keyboard.h Makefile raspimp.db raspimp.css raspimp.glade keyboard.glade
    scp raspimp.zip raspberrypi:/tmp
    rm raspimp.zip

    ssh -t raspberrypi '
        cd /tmp
        unzip raspimp.zip
        rm raspimp.zip
        mkdir -p /home/alarm/.config/raspimp
        cp raspimp.glade /home/alarm/.config/raspimp/raspimp.glade
        cp raspimp.db /home/alarm/.config/raspimp/raspimp.db
        cp raspimp.css /home/alarm/.config/raspimp/raspimp.css
        cp keyboard.glade /home/alarm/.config/raspimp/keyboard.glade
        make
        sudo -v
        killall raspimp && sudo cp raspimp /usr/bin/raspimp
        rm Makefile raspimp raspimp.c keyboard.c keyboard.h Makefile raspimp.db raspimp.css raspimp.glade keyboard.glade
    '
}

sqlite()
{
    if [ -f raspimp.db ]; then
        rm raspimp.db
    fi
    sqlite3 raspimp.db < raspimp.sql

}

database()
{
    scp raspimp.db raspberrypi:/home/alarm/.config/raspimp/
    ssh raspberrypi 'killall raspimp'
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
        s)
            sqlite
        ;;
        b)
            database
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
