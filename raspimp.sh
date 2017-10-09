#!/bin/bash

usage()
{
    echo "Rapimp helper script."
    echo
    echo "Usage: $0 [Options]"
    echo
    echo "Options:"
    printf "  %-20s %s\n" "-d" "deploy application"
    printf "  %-20s %s\n" "-s" "create sqlite database"
    printf "  %-20s %s\n" "-b" "deploy sqlite database"
    exit 1
}

deploy()
{
    if [ -f raspimp.zip ]; then
        rm raspimp.zip
    fi

    zip -r9 raspimp.zip raspimp.c Makefile
    scp raspimp.zip raspberrypi:/tmp
    rm raspimp.zip

    scp raspimp.db raspberrypi:/home/alarm
    scp raspimp.css raspberrypi:/home/alarm
    scp raspimp.glade raspberrypi:/home/alarm

    ssh -t raspberrypi '
        cd /tmp
        sudo chown root:root raspimp.zip
        sudo unzip raspimp.zip
        sudo rm raspimp.zip
        sudo make
        killall raspimp && sudo cp raspimp.o /usr/bin/raspimp
        sudo rm Makefile raspimp.c raspimp.o
    '
}

sqlite()
{
    if [ -f raspimp.db ]; then
        rm raspimp.db
    fi
    sqlite3 raspimp.db < create.sql

}

database()
{
    scp raspimp.db raspberrypi:/home/alarm
    ssh raspberrypi 'killall raspimp'
}

if [ $# == 0 ]; then
    usage
fi

while getopts "dsb" opt; do
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
        \?)
            usage
        ;;
        :)
            usage
        ;;
    esac
done

exit 0
