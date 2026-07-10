wifi_manager &
amixer set 19 255
amixer set 20 255
amixer set 21 255

amixer set 6 180
amixer set 7 180
amixer set 15 6

arecord -D hw:snddmic -r 48000 -f 16  -c 2 -o &
aplay -D hw:audiocodec  -r 48000 -f 16 -c 2 -o &
control_center & 
xiaozhi_gui &

