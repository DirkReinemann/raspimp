# raspimp

Music Player for Rasperry Pi 2 with Touchscreen.

## commands

### makefile

| command | description |
| - | - |
| make | compile project |
| make clean | remove compiled files, logs, temp files |
| make run | start project |
| make install | install project to /usr/share/raspimp and /usr/bin/raspimp |
| make uninstall | uninstall project |

### raspimp.sh

| command | description |
| - | - |
| ./raspimp.sh -d | deploy application to raspberry pi via ssh |
| ./raspimp.sh -v | start application on virtual display |

## dependencies

The following applications are used and should be installed.

  * ip
  * iwconfig
  * sqlite
