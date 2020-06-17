all:
	cc -o chompdrv chompdrv.c -lusb-1.0

C-USBIP:
	sudo usbip -a 127.0.0.1 1-1
