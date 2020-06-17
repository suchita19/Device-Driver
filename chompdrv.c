#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include<stdbool.h>


int uinp_fd;
struct uinput_user_dev uinp;

int setup_uinput_device() {
    int err=0;
    uinp_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(uinp_fd < 0) {
        printf("ERROR: INPUT DEVICE NOT FOUND\n");
        exit(-1);
    }
    
    
    err = ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
    if(err < 0) {
        printf("ERROR: IOCTL EV_KEY ERROR");
    }
    err = ioctl(uinp_fd, UI_SET_EVBIT, EV_SYN);
    if(err < 0) {
        printf("ERROR: IOCTL EVBIT ERROR\n");
    }
    ioctl(uinp_fd, UI_SET_KEYBIT, BTN_A);
   
    ioctl(uinp_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(uinp_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinp_fd, UI_SET_ABSBIT, ABS_Y);

    memset(&uinp, 0, sizeof(uinp));
    snprintf(uinp.name, UINPUT_MAX_NAME_SIZE, "ChompStick");
    uinp.id.bustype = BUS_USB;
    uinp.id.vendor = 0x9A7A;
    uinp.id.product = 0xBA17;
    uinp.id.version = 1;
    err = write(uinp_fd, &uinp, sizeof(uinp));
    if(err < 0) {
        printf("ERROR: UNABLE TO WRITE DEVICE.\n");
    }
    err = ioctl(uinp_fd, UI_DEV_CREATE);
    if(err < 0) {
        printf("ERROR: DEV_CREATE ERROR\n");
        exit(0);
    }
    return 0;
}

int closeDEV() {
    if(ioctl(uinp_fd, UI_DEV_DESTROY) < 0) {
        printf("ERROR: UNABLE TO CLOSE DEVICE\n");
        exit(0);
    }
    close(uinp_fd);
    return 1;
}


void emit(int fd, int type, int code, int val)
{
    struct input_event ie;
    
    ie.type = type;
    ie.code = code;
    ie.value = val;
    
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;
    
    write(fd, &ie, sizeof(ie));
}

void user_input(int xState, int yState, int buttonState) {
    if (xState == 1) {
        emit(uinp_fd, EV_ABS, ABS_X, -32767);
        emit(uinp_fd, EV_SYN, SYN_REPORT,0);
    }
    else if (xState == 3) {
        emit(uinp_fd, EV_ABS, ABS_X, 32767);
        emit(uinp_fd, EV_SYN, SYN_REPORT,0);
    }
    else if (xState == 2) {
        emit(uinp_fd, EV_ABS, ABS_X, 0);
        emit(uinp_fd, EV_SYN, SYN_REPORT, 0);
    }
    if (buttonState == 1) {
        emit(uinp_fd, EV_KEY, BTN_A, 1);
        emit(uinp_fd, EV_SYN, SYN_REPORT, 0);
    }
    else if (buttonState == 0) {
        emit(uinp_fd, EV_KEY, BTN_A, 0);
        emit(uinp_fd, EV_SYN, SYN_REPORT, 0);
    }
    if (yState == 1) {
        emit(uinp_fd, EV_ABS, ABS_Y, 32767);
        emit(uinp_fd, EV_SYN, SYN_REPORT,0);
    }
    else if (yState == 3) {
        emit(uinp_fd, EV_ABS, ABS_Y, -32767);
        emit(uinp_fd, EV_SYN, SYN_REPORT,0);
    }
    else if (yState == 2) {
        emit(uinp_fd, EV_ABS, ABS_Y, 0);
        emit(uinp_fd, EV_SYN, SYN_REPORT, 0);
    }
}

void error(char* s, int err) { 
    printf("%s", s);
    printf("Error: ");
    printf("%s", libusb_error_name(err));
    printf("\n");
    exit(err);
}

int main() {
    
    int i = 0;
    i = setup_uinput_device();
    if(i < 0) {
        printf("ERROR: SETUP FAILURE\n");
        return -1;
    }

    int transfer_size;
    int err;
    unsigned char data[1];
    libusb_device_handle* dev;
    libusb_init(NULL);
    dev = libusb_open_device_with_vid_pid(NULL, 0x9A7A, 0xBA17);
    if (dev == NULL) {
        printf("ERROR: DEVICE NOT FOUND. PLEASE USE USBIP AND TRY AGAIN.");
        return -1;
    }
    
    err = libusb_claim_interface(dev, 0);
	if (err) {
	error("Cannot claim interface!", err);
	}
    err = libusb_set_interface_alt_setting(dev,0,0);
	if (err) {
		error("Cannot set alternate setting!", err);
	}
    while (1) {
        err = libusb_interrupt_transfer(dev, 0x81, data, sizeof(data), &transfer_size, 1000);
        if (err) {
            error("Interrupt IN Transfer Failed!", err);
        }
        unsigned char byte = data[0];
        unsigned char m = 1;
        unsigned char bits[8];
        int i, j = CHAR_BIT-1;
        for ( i = 0; i < 8; i++,j--,m = 1) {
            bits[i] = (byte & (m<<=j)) != NULL;
        }
        int x, y, button;
        button = bits[3];
        y = bits[7] + 2*bits[6];
        x = bits[5] + 2*bits[4];
        int XState = 2*bits[4] + bits[5];
        int YState = 2*bits[6] + bits[7];
        int ButtonState = bits[3];
        user_input(XState, YState, ButtonState);
    }
    err = closeDEV();
}

