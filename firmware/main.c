/*
 *    Filename: main.c
 * Description: bus interface between PC and MDB-bus-device
 *     License: GPLv2 or later
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <avr/wdt.h>

#include "./main.h"

#include "./io.h"
#include "./uart.h"
#include "./task_hopper.h"

#define LED_DDR DDRB				// Hier DDR der Test-LED einstellen!
#define LED_PORT PORTB				// Hier PORT der Test-LED einstellen!
#define LED_AMBER_PIN PB0				// Hier PIN der Test-LED einstellen!
#define LED_BLUE_PIN PB1
#define LED_AMBER(x) out(LED_PORT,LED_AMBER_PIN,0,x)	// DO NOT CHANGE
#define LED_BLUE(x) out(LED_PORT,LED_BLUE_PIN,0,x)	// DO NOT CHANGE




uint8_t hexNibbleToAscii(uint8_t x);
uint8_t asciiToHex(uint8_t high, uint8_t low);

void __attribute__((noreturn)) fatal_real(const char * PROGMEM str, uint8_t d) {
	uartPC_tx_pstr(PSTR("\nFATAL: "));
	uartPC_tx_pstr(str);
	uartPC_tx_blocking(hexNibbleToAscii(d>>4));
	uartPC_tx_blocking(hexNibbleToAscii(d&0xF));
	uartPC_tx_blocking('\n');
	while (1) {}
}

#include "databuf.h"

typedef enum { CONSTANT, BLINK, TIMEOUT } rgb_mode_enum;
rgb_mode_enum rgb_led_mode[2] = {TIMEOUT, TIMEOUT};
uint8_t rgb_led_red[2] = {100, 100};
uint8_t rgb_led_blue[2] = {100, 100};
uint8_t rgb_led_green[2] = {100, 100};
uint32_t rgb_led_timer[2]= {0, 0}; 


rgb_mode_enum asciiToRGBMode(uint8_t x);


uint8_t asciiNibbleToHex(uint8_t ascii) {
	if (('0' <= ascii) && (ascii <= '9')) {
		return ascii-'0';
	} else if (('A' <= ascii) && (ascii <= 'F')) {
		return ascii-'A'+10;
	} else {
		FATAL_h("invalid hexstring",ascii);
		return 0;
	}
}

uint8_t asciiToHex(uint8_t high, uint8_t low) {
	return (asciiNibbleToHex(high) << 4) | asciiNibbleToHex(low);
}

uint8_t hexNibbleToAscii(uint8_t x) {
	if (x<10) {
		return '0'+x;
	} else if (x<16) {
		return 'A' + x - 10;
	} else {
		FATAL("hex too large");
		return 0;
	}
}


void task_comm(void) {
	static databuf cmd, resp; // initialised to zero because they are static variables, otherwise databufReset(&cmd), ...(&resp) would have to be called! see http://stackoverflow.com/a/4081193
	
	static uint8_t cmdBytesSent=0;
	static uint8_t respChk=0;
	static uint8_t respLen=0;
	static uint8_t timeout=0;

	static enum {READ_CMD, PROCESS_MDB_CMD, PROCESS_EXTENSION_CMD, READ_MDB_RESP, SEND_RESP_TO_PC}
		state = READ_CMD;
	
	LED_BLUE(state != READ_CMD); // state busy	
	switch (state) {
		case READ_CMD: // get command from PC
			if (uartPC_rx_ready()) {
				uint8_t d=uartPC_rx();
				// uartPC_tx_blocking(d); // als "loopback" test - PC-Software ist nicht dafür gedacht, also nur für händisches Testen einkommentieren
				if (d=='\r' || d=='\n') {
					if (hasData(&cmd) && (peekData(&cmd) == 'X')) {
						state = PROCESS_EXTENSION_CMD;
					} else {
						state=PROCESS_MDB_CMD;
					}
					cmdBytesSent=0;
				} else {
					storeData(&cmd,d);
				}
			}
			break;
		case PROCESS_EXTENSION_CMD:
		case PROCESS_MDB_CMD: // send command to device
			if (uartPC_rx_ready() || uartBus_rx_ready()) {
				FATAL("rx while SEND CMD");
			}
			if (state == PROCESS_MDB_CMD) {
				if (uartBus_tx_ready()) {
					if (hasData(&cmd)) {
						// send databyte. set bit8 if it is the first byte
						uartBus_tx(readAsciiHexbyte(&cmd), (cmdBytesSent==0));
						cmdBytesSent++;
					} else {
						databufReset(&cmd);
						// response R:<hexdata>
						storeData(&resp,'R');
						storeData(&resp,':');
						state=READ_MDB_RESP;
						respChk=0;
						respLen=0;
						timeout=0;
					}
				}
			} else if (state == PROCESS_EXTENSION_CMD) {
				readData(&cmd); // discard "X"
				uint8_t tmp = readData(&cmd);
				if (tmp == 'L') {
					/**
					 * LED Protocol:
					 * 
					 * command: LrrggbbMrrggbbM
					 * where rr, gg, bb are hex numbers 00...FF for the red/green/blue intensity
					 * and M is the LED mode, N for normal always on, B for blinking, T for timeout = switch-off after ca. 20 seconds
					 * 
					 * the first block rrggbbM is for LED 0, the second for LED 1
					 */
					// LED command
					for (uint8_t i=0; i<=1; i++) {
						rgb_led_red[i] = readAsciiHexbyte(&cmd);
						rgb_led_green[i] = readAsciiHexbyte(&cmd);
						rgb_led_blue[i] = readAsciiHexbyte(&cmd);
						rgb_mode_enum newMode = asciiToRGBMode(readData(&cmd));
						if (!((rgb_led_mode[i] == newMode) && (rgb_led_mode[i] == BLINK)))  {
							// reset timer, but not for an unchanged blink mode (then the sequence looks stupid)
							rgb_led_timer[i] = 0;
						}
						rgb_led_mode[i] = newMode;
					}
					if (hasData(&cmd)) {
						FATAL("extra data at end of LED cmd");
					}
					storeData(&resp, 'R');
					storeData(&resp, ':');
					storeData(&resp, 'O');
					storeData(&resp, 'K');
					state = SEND_RESP_TO_PC;
				} else if (tmp == 'H') {
					/**
					 * Hopper Protocol:
					 * 
					 * command: H
					 * 
					 * response:
					 * A  = ACK: command received, starting a dispense operation, please resend to poll for the result
					 *  B = busy, please resend command until you receive something else than busy (must not take more than 3 seconds)
					 *  E01  = out of service because of a serious error #01.
					 *         01 is the hexadecimal error number from hopperErrorEnum in task_hopper.h
					 *         Please reset board to exit this state, otherwise all hopper requests will be ignored and answered with this error.
					 *  RD = okay, dispensed a coin
					 *  RE = okay, hopper is empty, could not dispense a coin
					 */
					// hopper command (PC repeats this until the reply is not "busy")
					storeData(&resp, 'R');
					storeData(&resp, ':');
					if (hopper_get_error() != HOPPER_OKAY) {
						storeData(&resp, 'E'); // error, will not payout any coins anymore.
						storeHexNibbleAsAscii(&resp, (uint8_t) hopper_get_error());
					} else if (hopper_busy()) {
						storeData(&resp,'B'); // busy, please request again.
					} else if (hopper_has_response()) {
						storeData(&resp,'R');
						storeData(&resp, hopper_get_response());
					} else {
						hopper_request_dispense();
						storeData(&resp, 'A'); // ACK
					}
					state = SEND_RESP_TO_PC;
				} else {
					FATAL("invalid extension cmd");
				}
			}
			break;
		case READ_MDB_RESP: // receive response from device, send ACK
			if (uartPC_rx_ready()) {
				FATAL("rx0 while READ RESP");
			}
			if (uartBus_rx_ready()) {
				timeout=0;
				uint16_t d=uartBus_rx();
				respLen++;
				storeHexNibbleAsAscii(&resp, d);
				
				if (d&(1<<8)) { // bit8 is set - this is the end of the response
					uint8_t okay=0;
					uint8_t receivedNAK=0;
					state=SEND_RESP_TO_PC;
					if (respLen==1) {
						if (d==0x100) {
							okay=1; // ACK
						} else if (d==0x1FF) {
							// NAK empfangen - nicht mit NAK antworten!
							receivedNAK=1;
						} else {
							// Unsinn empfangen
						}
						okay = (d == 0x100); // ACK empfangen?
					} else {
						// answer contains data - validate checksum
						okay = ((d & 0xff) == respChk);
					}
					
					if (okay) {
						if (respLen>1) {
							// ACK nur wenn die Gegenstelle mehr zu sagen hatte als "ACK"
							uartBus_tx(0x00,0); // ACK
						}
					} else {
						databufReset(&resp);
						storeData(&resp,'R');
						if (receivedNAK) {
							// NAK empfangen - nicht mit NAK antworten!
							storeData(&resp,'N');
						} else {
							storeData(&resp,'E');
							uartBus_tx(0xFF,0);
						}
					}
				}
				
				respChk += (d&0xFF);

			} else {
				// uartBus_rx not ready
				timeout++;
				if (timeout >= 50) {
					// took longer than 50ms/TICKS_PER_MS ~= 8ms
					databufReset(&resp);
					storeData(&resp,'R');
					storeData(&resp,'T');
					state=SEND_RESP_TO_PC;
				}
			}
			break;
		case SEND_RESP_TO_PC: // send response to PC
			if (uartPC_rx_ready() || uartBus_rx_ready()) {
				FATAL("rx while sending");
			}
			if (uartPC_tx_ready()) {
				if (hasData(&resp)) {
					uartPC_tx(readData(&resp));
				} else {
					uartPC_tx('\n');
					databufReset(&resp);
					state=READ_CMD;
				}
			}
			break;
		default:
			FATAL("unknown state");
	}
}

void init_timer(void) {
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00); // non-inverting fast PWM
	TCCR0B = (1 << CS01); // use system clock / 8
	
	TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM20); // non-inverting fast PWM
	TCCR2B = (1 << CS21); // use system clock / 8

	// Timer1 is 16bit and a little different
	// 8bit *inverting* fast PWM
	// WORKAROUND: Atmel fast PWM does not go down to 0%, but still outputs a narrow spike at 0.
	// 255 = 100% is possible, so we use inverted mode.
	TCCR1A = (1 << COM1A0) | (1 << COM1A1) | (1 << COM1B0) | (1 << COM1B1) | (1 << WGM10); // non-inverting fast PWM
	TCCR1B = (1 << WGM12)  | (1 << CS11); // use system clock / 8
	TIFR1 = (1<<TOV1); // clear overflow on first run
}

// set RGB LED no. $index
void rgb_set_pwm(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
	r=(r*alpha)>>8;
	g=(g*alpha)>>8;
	b=(b*alpha)>>8;
	if (index == 1) {
		OCR2A=r;
		OCR0A=g;
		OCR0B=b;
	} else {
		OCR1B=255-r; // inverted PWM mode! see init_timer()
		OCR1A=255-g; // inverted PWM mode! see init_timer()
		OCR2B=b;
	}
}

/************************************
 * RGB LED
 ***********************************/

rgb_mode_enum asciiToRGBMode(uint8_t x) {
	if (x=='N') {
		return CONSTANT;
	} else if (x=='B') {
		return BLINK;
	} else if (x=='T') {
		return TIMEOUT;
	} else {
		FATAL("invalid LED mode");
		return CONSTANT;
	}
}

void task_rgb_led(void) {
	// slow down blinking / fadeout by 2^(n+1). n=0 means 2*255/TICKS_PER_MS = 80 milliseconds 
	const uint8_t BLINK_FREQUENCY_DIVIDER=3;
	const uint8_t TIMEOUT_FADEOUT_DIVIDER=4;
	
	const uint8_t TIMEOUT_SECONDS = 20; // how many seconds until fadeout?
	
	const uint32_t FADEOUT_START= TIMEOUT_SECONDS * (uint32_t)TICKS_PER_S;
	
	// save CPU time by only processing one RGB channel per task call
	// this slows down the LED update rate by factor 2
	static uint8_t index = 0;
	index = !index;
	uint8_t alpha=255;
	
	switch (rgb_led_mode[index]) {
		case CONSTANT:
			alpha=255;
			break;
		case BLINK:
			rgb_led_timer[index]++;
			if (rgb_led_timer[index] < (256 << BLINK_FREQUENCY_DIVIDER)) {
				alpha=rgb_led_timer[index]>>BLINK_FREQUENCY_DIVIDER;
			} else {
				alpha=255-(rgb_led_timer[index]>>BLINK_FREQUENCY_DIVIDER);
			}
			if (rgb_led_timer[index] >= ((255+256) << BLINK_FREQUENCY_DIVIDER)) {
				rgb_led_timer[index]=0;
			}
			break;
		case TIMEOUT:
			if (rgb_led_timer[index] < FADEOUT_START) {
				// phase 1: stay on fully
				rgb_led_timer[index]++;
				alpha = 255;
			} else if (rgb_led_timer[index] < FADEOUT_START + (255 << TIMEOUT_FADEOUT_DIVIDER)) {
				// phase 2: slowly fade to black
				rgb_led_timer[index]++;
				alpha = 255 - ((rgb_led_timer[index] - FADEOUT_START) >> TIMEOUT_FADEOUT_DIVIDER);
			} else {
				// phase 3: stay off
				alpha = 0;
			}
			break;
	}
	rgb_set_pwm(index, rgb_led_red[index], rgb_led_green[index], rgb_led_blue[index], alpha);
}








void sleep_remaining_time(void) {
	if (TIFR1 & (1<<TOV1)) {
		FATAL("time overrun");
	}
	LED_AMBER(1); // idle
	while (!(TIFR1 & (1<<TOV1))) {
		// wait until timer overflow
	}
	LED_AMBER(0);
	TIFR1 = 1 << TOV1; // reset overflow flag for the next time
	if (TIFR1 & (1<<TOV1)) {
		FATAL("TOV reset err");
	}
}

int main(void) {
	const uint8_t TEST=0;
	if (!TEST) {
		wdt_enable(WDTO_120MS);
	}
	LED_DDR |= (1<<LED_AMBER_PIN) | (1<<LED_BLUE_PIN); 		// setting LED-PIN to output
	LED_PORT |= (1<<LED_AMBER_PIN);
        
	uartPC_init();
	uartBus_init();
        hopper_init();
	// UART DDRs:
	DDRD |= (1<<PD3) | (1<<PD1);
        
        
        // RGB LEDs switched on at start, later overriden by timer PWM output
        DDRD |= (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7);
//  	PORTD |= (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7);
        DDRB |= (1<<PB3) | (1<<PB4);
//           PORTB |= (1<<PB3) | (1<<PB4);
        init_timer();
	// send one byte so that txReady() works
	uartPC_tx('\n');
	uartBus_tx(0xFF,1);
	
	if (TEST) {
		LED_PORT ^= (1<<LED_AMBER_PIN);
		delayms(1000);
		LED_PORT ^= (1<<LED_AMBER_PIN);
		delayms(1000);
		
		uartPC_tx_pstr(PSTR("TEST active\n"));     
		while (1) {
 			LED_PORT ^= (1<<LED_AMBER_PIN);
			if (uartPC_rx_ready()) {
				uint8_t d=uartPC_rx();
				uartPC_tx_pstr(PSTR("RX:"));
				uartPC_tx_blocking(d); 
				uartPC_tx_blocking('\n'); 
			} else {
				//uartPC_tx_pstr(PSTR("."));
			}
		}
	}
	
	uartPC_tx_pstr(PSTR("INFO:booting\n"));   
	
	init_timer();
	
        while(1) {
		wdt_reset();
                // ATTENTION, no delays allowed in this loop.
		// fixed loop runtime is (0xFF * timer1 prescaler)/F_CPU = 170µs with prescaler=8 and F_CPU=12MHz
		// sleep_remaining_time() eats the remaning time.
		// UART buffer overruns would occur at a runtime of more than 8bit/(38.4kbit/s)=208µs
		
                task_comm();
		task_rgb_led();
		// TODO hopper untested
#warning TODO hopper disabled
		//task_hopper();
		
                // TODO sleep for...
                sleep_remaining_time();
	}
	return 0;
}
