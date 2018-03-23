// most came from https://github.com/libopencm3/libopencm3-examples/tree/master/examples/stm32/f1/other/usb_cdcacm
// altered to implement an UART type thing 
//


/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "buspirateNG.h"
#include "debug.h"
#include "cdcacm.h"
#include "usbdescriptor.h"

// globals
static usbd_device *my_usbd_dev;		// usb device
static uint8_t configured;
// first cdc endpoint
static volatile uint8_t rxbuff1[RXBUFFSIZE];	// intermediate receive buffer
static volatile uint8_t txbuff1[TXBUFFSIZE];	// intermediate transmit bufff
static volatile uint8_t	rxhead1;		// pointers to position in buffers
static volatile uint8_t	txhead1;
static volatile uint8_t	rxtail1;
static volatile uint8_t	txtail1;
// second cdc endpoint
static volatile uint8_t rxbuff2[RXLABUFFSIZE];	// intermediate receive buffer
static volatile uint8_t txbuff2[TXLABUFFSIZE];	// intermediate transmit bufff
static volatile uint8_t	rxhead2;		// pointers to position in buffers
static volatile uint8_t	txhead2;
static volatile uint8_t	rxtail2;
static volatile uint8_t	txtail2;
uint8_t usbd_control_buffer[256];		// Buffer to be used for control requests.
static char  printbuf[PRINTBUFLEN];		// used as printf buffer
// usbserialnumber
char usbserial[25];

static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

configured=1;

	switch(req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		char local_buf[10];
		struct usb_cdc_notification *notif = (void *)local_buf;

		/* We echo signals back to host as notification. */
		notif->bmRequestType = 0xA1;
		notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
		notif->wValue = 0;
		notif->wIndex = 0;
		notif->wLength = 2;
		local_buf[8] = req->wValue & 3;
		local_buf[9] = 0;
		// usbd_ep_write_packet(0x83, buf, 10);
		return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_SET_LINE_CODING: 
		if(*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;

		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_tx_cb1(usbd_device *usbd_dev, uint8_t ep)
{
	(void)usbd_dev;
	(void)ep;
}


static void cdcacm_data_rx_cb1(usbd_device *usbd_dev, uint8_t ep)
{
	int i=0;

	(void)ep;

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	while (len)
	{
		rxbuff1[rxhead1]=buf[i++];
		rxhead1=(rxhead1+1)&(RXBUFFSIZE-1);
		len--;
	}
}

static void cdcacm_data_tx_cb2(usbd_device *usbd_dev, uint8_t ep)
{
	(void)usbd_dev;
	(void)ep;
}


static void cdcacm_data_rx_cb2(usbd_device *usbd_dev, uint8_t ep)
{
	int i=0;

	(void)ep;

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x03, buf, 64);

	while (len)
	{
		rxbuff2[rxhead2]=buf[i++];
		rxhead2=(rxhead2+1)&(RXLABUFFSIZE-1);
		len--;
	}
}


// first cdc channel (usermode)
uint8_t cdcbyteready(void)
{
	return !(rxhead1==rxtail1);
}

uint8_t cdcgetc(void)
{
	uint8_t c;

	while(rxhead1==rxtail1);				// block until data available

	c=rxbuff1[rxtail1];
	rxtail1=(rxtail1+1)&(RXBUFFSIZE-1);
	return c;	
}

void cdcputc(char c)
{
	txbuff1[txhead1]=c;
	txhead1=(txhead1+1)&(TXBUFFSIZE-1);
}

void cdcputs(char *s)
{
	while(*s) cdcputc(*(s++));
}

void cdcprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(printbuf, PRINTBUFLEN, fmt, args);
	cdcputs(printbuf);
	va_end(args);
}



// second cdc channel (binmode)
uint8_t cdcbyteready2(void)
{
	return !(rxhead2==rxtail2);
}

uint8_t cdcgetc2(void)
{
	uint8_t c;

	while(rxhead2==rxtail2);				// block until data available

	c=rxbuff2[rxtail2];
	rxtail2=(rxtail2+1)&(RXLABUFFSIZE-1);
	return c;	
}

void cdcputc2(char c)
{
	txbuff2[txhead2]=c;
	txhead2=(txhead2+1)&(TXLABUFFSIZE-1);
}

void cdcputs2(char *s)
{
	while(*s) cdcputc2(*(s++));
}

void cdcprintf2(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(printbuf, PRINTBUFLEN, fmt, args);
	cdcputs2(printbuf);
	va_end(args);
}



static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb1);
	usbd_ep_setup(usbd_dev, 0x81, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb1);
	usbd_ep_setup(usbd_dev, 0x03, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb2);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb2);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);
	usbd_ep_setup(usbd_dev, 0x84, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);

//	configured=1;
}

uint8_t usbready(void)
{
 	return configured;
}

//void  usb_lp_can_rx0_isr(void)

// polls the usb for new data and sends data back if available
void cdcpoll(void)
{
	int i;
	uint8_t buf[64];


	if(my_usbd_dev!=NULL) usbd_poll(my_usbd_dev);

	//move to somewhere else??
	if(txtail1!=txhead1)							// we have data to send?
	{
		gpio_set(BP_USB_LED_PORT, BP_USB_LED_PIN);			// flash led during TX

		i=0;
		while((txtail1!=txhead1)&&(i<63))
		{
			buf[i++]=txbuff1[txtail1];
			txtail1=(txtail1+1)&(TXBUFFSIZE-1);
		}
		buf[i+1]=0;
		while((usbd_ep_write_packet(my_usbd_dev, 0x81, buf, i)==0));	// try resending until it is succeeded

		gpio_clear(BP_USB_LED_PORT, BP_USB_LED_PIN);
	}
	//move to somewhere else??
	if(txtail2!=txhead2)							// we have data to send?
	{
		gpio_set(BP_USB_LED_PORT, BP_USB_LED_PIN);			// flash led during TX

		i=0;
		while((txtail2!=txhead2)&&(i<63))
		{
			buf[i++]=txbuff2[txtail2];
			txtail2=(txtail2+1)&(TXLABUFFSIZE-1);
		}
		buf[i+1]=0;
		while((usbd_ep_write_packet(my_usbd_dev, 0x83, buf, i)==0));	// try resending until it is succeeded

		gpio_clear(BP_USB_LED_PORT, BP_USB_LED_PIN);
	}
}


// init all the buffers and start the usb bus
void cdcinit(void)
{
	int i;
	uint16_t *id = (uint16_t *)0x1FFFF7E8;			// unique 96bit id location

	my_usbd_dev=NULL;
	// first cdc channel
	for(i=0; i<RXBUFFSIZE; i++) rxbuff1[i]=0x0;
	for(i=0; i<TXBUFFSIZE; i++) txbuff1[i]=0x0;
	rxhead1=0;
	txhead1=0;
	rxtail1=0;
	txtail1=0;

	for(i=0; i<RXBUFFSIZE; i++) rxbuff2[i]=0x0;
	for(i=0; i<TXBUFFSIZE; i++) txbuff2[i]=0x0;
	rxhead2=0;
	txhead2=0;
	rxtail2=0;
	txtail2=0;

	configured=0;

	//create something unique (some bitshuffling to match the same outut as i command)
	// using 32 bits will trigger a warning
	snprintf(usbserial, 25, "%04X%04X%04X%04X%04X%04X", id[1], id[0], id[3], id[2], id[5], id[4]);

	// setup usb
	my_usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(my_usbd_dev, cdcacm_set_config);

	//nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, IRQ_PRI_USB);
	//nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}



