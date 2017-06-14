#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include <string.h>
#include "usbdrv.h"
#include "usart.h"
#include "hc05.h"

volatile char temp = 0, count = 0 ;
char x_disp = 0, y_disp = 0, click = 0, ready = 1;

ISR (USART_RXC_vect) {
	count++;
	temp = UDR;
	if (count==1) {
		if ((temp > 5) || (temp < -5)) {
			if (temp < -5) {
				x_disp = temp + 5;
			}
			else if (temp > 5) {
				x_disp = temp - 5;
			}
		}
	}
	else if (count==2) {
		if ((temp > 5) || (temp < -5)) {
			if (temp < -5) {
				y_disp = temp + 5;
			}
			else if (temp > 5) {
				y_disp = temp - 5;
			}
		}
	}
	else if (count==3) {
		click = temp;
		count = 0;
	}
	PORTA &= ~(1<<PA0);
	ready = 0;
}
/* ================ USB interface ================ */
PROGMEM const char usbHidReportDescriptor[50] = { /* USB report descriptor, size must match usbconfig.h */
	0x05, 0x01,				//	USAGE_PAGE (Generic Desktop)
	0x09, 0x02,				//	USAGE (Mouse)
	0xa1, 0x01,				//	COLLECTION (Application)
	0x09, 0x01,				//		USAGE (Pointer)
	0xa1, 0x00,				//		COLLECTION (Physical)
	0x05, 0x09,				//			USAGE_PAGE (Button)
	0x19, 0x01,				//			USAGE_MINIMUM (Button 1)
	0x29, 0x02,				//			USAGE_MAXIMUM (Button 2)
	0x15, 0x00,				//			LOGICAL_MINIMUM (0)
	0x25, 0x01,				//			LOGICAL_MAXIMUM (1)
	0x95, 0x02,				//			REPORT_COUNT (2)
	0x75, 0x01,				//			REPORT_SIZE (1)
	0x81, 0x02,				//			INPUT (Data,Var,Abs)
	0x95, 0x01,				//			REPORT_COUNT (1)
	0x75, 0x06,				//			REPORT_SIZE (6)
	0x81, 0x03,				//			INPUT (Cnst,Var,Abs)
	0x05, 0x01,				//			USAGE_PAGE (Generic Desktop)
	0x09, 0x30,				//			USAGE (X)
	0x09, 0x31,				//			USAGE (Y)
	0x15, 0x81,				//			LOGICAL_MINIMUM (-127)
	0x25, 0x7f,				//			LOGICAL_MAXIMUM (127)
	0x75, 0x08,				//			REPORT_SIZE (8)
	0x95, 0x02,				//			REPORT_COUNT (2)
	0x81, 0x06,				//			INPUT (Data,Var,Rel)
	0xc0,					//		END_COLLECTION
	0xc0					//	END_COLLECTION
};
/* The data described by this descriptor consists of 4 bytes:
 *      .  .  .  .  .  . B1 B0 .... one byte with mouse button states
 *     X7 X6 X5 X4 X3 X2 X1 X0 .... 8 bit signed relative coordinate x
 *     Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 .... 8 bit signed relative coordinate y
 */

typedef struct{
	char	buttonMask;
	char	dx;
	char	dy;
}report_t;
static report_t reportBuffer;
static uchar	idleRate;	/* repeat rate for keyboards, never used for mice */

static void updateReport(void) {
	reportBuffer.dx = x_disp;
	reportBuffer.dy = y_disp;
	reportBuffer.buttonMask = click;
	x_disp = y_disp = click = 0;
}

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t    *rq = (void *)data;
	/* The following requests are never used. But since they are required by
	 * the specification, we implement them in this example.
	 */
	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {    /* class request type */
		if (rq->bRequest == USBRQ_HID_GET_REPORT) {  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			usbMsgPtr = (void *)&reportBuffer;
			return sizeof(reportBuffer);
		}
		else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
			usbMsgPtr = &idleRate;
			return 1;
		}
		else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
			idleRate = rq->wValue.bytes[1];
		}
	}
	else {
		/* no vendor specific requests implemented */
	}
	return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void) {
	DDRA |= (1<<PA0);
	PORTA |= (1<<PA0);

	uchar   i;
	usart_init();
	wdt_enable(WDTO_1S);
	usbInit();
	usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
	i = 0;
	while (--i) {             /* fake USB disconnect for > 250 ms */
		wdt_reset();
		_delay_ms(1);
	}
	usbDeviceConnect();

	sei();
	for (;;) {
		if (ready) {
			hc_05_bluetooth_transmit_byte('a');
		}
		wdt_reset();
		usbPoll();
		if(usbInterruptIsReady()){
			updateReport();
			usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
		}
	}
}