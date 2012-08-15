solar: solar.c
	$(CC) -g -std=c99 -o solar solar.c -I/usr/include/libusb-1.0  -lusb-1.0
