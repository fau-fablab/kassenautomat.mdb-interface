/*
 *    Filename: task_hopper.c
 * Description: control task for a "dumb" Money Controls Compact Hopper
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


#include "task_hopper.h"
#include "./io.h"
#include "./main.h"

hopperResponseEnum hopperResponse = HOPPER_RESPONSE_ALREADY_SENT;
uint8_t hopperDoDispense=0;
hopperErrorEnum hopperError = HOPPER_OKAY;

#warning TODO hopper IO pins

#define HOPPER_PORT_MOTOR PORTC
#define HOPPER_DDR_MOTOR DDRC
#define HOPPER_PIN_MOTOR PC5

#define HOPPER_DDR_OPTOSUPPLY DDRA
#define HOPPER_PIN_OPTOSUPPLY PA0
#define HOPPER_PORT_OPTOSUPPLY PORTA

#define HOPPER_INPORT_SENSOR PINC
#define HOPPER_PIN_SENSOR PC4


inline void hopperOptoSupply(uint8_t on) {
	out(HOPPER_PORT_OPTOSUPPLY,HOPPER_PIN_OPTOSUPPLY,0,on);
}

inline void hopperMotor(uint8_t on) {
	out(HOPPER_PORT_MOTOR,HOPPER_PIN_MOTOR,0,on);
}

inline uint8_t hopperCoinPresent(void) {
	return in(HOPPER_INPORT_SENSOR, HOPPER_PIN_SENSOR, 0);
}

inline void hopper_init(void) {
	hopperMotor(0);
	setBit(HOPPER_DDR_MOTOR,HOPPER_PIN_MOTOR,1);
	setBit(HOPPER_DDR_OPTOSUPPLY,HOPPER_PIN_OPTOSUPPLY,1);
	hopperOptoSupply(1);
}


hopperErrorEnum hopper_get_error(void) {
	return hopperError;
}

uint8_t hopper_busy(void) {
	return (hopperResponse == HOPPER_RESPONSE_BUSY) || hopperDoDispense;
}

uint8_t hopper_has_response(void) {
	return (hopperResponse != HOPPER_RESPONSE_ALREADY_SENT) && (hopperResponse != HOPPER_RESPONSE_BUSY) && (!hopperDoDispense);
}

hopperResponseEnum hopper_get_response(void) {
	if (hopperResponse == HOPPER_RESPONSE_ALREADY_SENT) {
		FATAL("hopper response read twice");
	}
	hopperResponseEnum r=hopperResponse;
	hopperResponse=HOPPER_RESPONSE_ALREADY_SENT;
	return r;
}

void hopper_request_dispense(void) {
	if (hopper_busy() || hopper_has_response()) {
		FATAL("unfinished hopper request");
	}
	hopperDoDispense=1;
}

void task_hopper(void) {
	static enum {HOPPER_STATE_INIT, HOPPER_STATE_CHECK_SENSOR_1, HOPPER_STATE_CHECK_SENSOR_2,
		HOPPER_STATE_DISPENSE_START, HOPPER_STATE_DISPENSE_MIDDLE, HOPPER_STATE_DISPENSE_DETECTED_COIN, 
		HOPPER_STATE_DISPENSE_SUCCESS_FINISH, HOPPER_STATE_COOLDOWN} 
		hopperState=HOPPER_STATE_INIT;
	// how many ticks should the execution of the statemachine be paused (e.g. to wait for IO to settle)
	// this saves us a lot of intermediate "do nothing and wait" states
		
	static uint16_t hopperPause=100*TICKS_PER_MS; // at the very first start, wait 100ms so that there are no error reports during startup
	static uint16_t hopperTimeout=0; // state-specific counter to wait for some event (the statemachine is still executed each time)
	
	if (hopperError != HOPPER_OKAY) {
		// disabled because of error
		hopperMotor(0);
		return;
	}
	
	if (hopperPause > 0) {
		hopperPause--;
		// still waiting, nothing to do yet
		return;
	}
	
#warning TODO Fotodiode tauschen!
	switch (hopperState) {
		case HOPPER_STATE_INIT:
			// wait for request. no coins may appear at the sensor now.
			if (hopperCoinPresent()) {
				hopperError=HOPPER_ERR_UNEXPECTED_COIN;
			}
			if (!hopperDoDispense) {
				// no dispense requested, nothing to do
				return;
			}
			hopperDoDispense = 0; // reset request
			hopperResponse = HOPPER_RESPONSE_BUSY;
			// init, switch off opto supply (sensor will always report coin present), wait 10ms
			hopperState=HOPPER_STATE_CHECK_SENSOR_1;
			hopperMotor(0);
			hopperOptoSupply(0);
			hopperPause=10*TICKS_PER_MS;
			break;
		case HOPPER_STATE_CHECK_SENSOR_1:
			// check if sensor high (= pullup okay)
			if (!hopperCoinPresent()) {
				hopperError=HOPPER_ERR_SENSOR1;
				break;
			}
			// switch on opto supply, wait 10ms
 			hopperState=HOPPER_STATE_CHECK_SENSOR_2;
			hopperTimeout=0;
			hopperOptoSupply(1);
			hopperPause=10*TICKS_PER_MS;
			break;
		case HOPPER_STATE_CHECK_SENSOR_2:
			// sensor should report "no coin" for 250ms before a payout is allowed
			if (hopperCoinPresent()) {
				hopperError=HOPPER_ERR_SENSOR2;
				return;
			}
			if (hopperTimeout < 250*TICKS_PER_MS) {
				// wait until time has elapsed
				hopperTimeout++;
				break;
			}
			if (hopperDoDispense) {
				hopperMotor(1);
				// after 3ms of undefined state, no coin may be present for 25ms.
				hopperPause=3*TICKS_PER_MS; // Datasheet: "Tu"
				hopperTimeout=25*TICKS_PER_MS; // Datasheet: "Tn"
				hopperState=HOPPER_STATE_DISPENSE_START;
			}
		case HOPPER_STATE_DISPENSE_START:
			// start of dispensing
			// no coin may be detected yet
			if (hopperCoinPresent()) {
				hopperError=HOPPER_ERR_EARLY_COIN;
				return;
			}
			// wait
			if (hopperTimeout > 0) {
				hopperTimeout--;
			} else {
				// now the coin may be detected
				hopperState=HOPPER_STATE_DISPENSE_MIDDLE;
				hopperTimeout=0;
			}
		case HOPPER_STATE_DISPENSE_MIDDLE:
			// middle of dispensing - expect coin
			hopperTimeout++;
			
			if (hopperCoinPresent()) {
				// coin found!
				// after 1ms of undefined state (debounce), "coin present" must be on for at least 5ms
				hopperPause=1*TICKS_PER_MS;
				hopperState=HOPPER_STATE_DISPENSE_DETECTED_COIN;
				hopperTimeout=0;
			} else if (hopperTimeout < 800*TICKS_PER_MS) {
				// wait for coin...
			} else if (hopperTimeout == 800*TICKS_PER_MS) {
				// we waited for a coin at least 800ms
				// then turn off motor
				hopperMotor(0);
				// ignore signals for 1ms against EMI trouble
				hopperPause=1*TICKS_PER_MS;
			} else if (hopperTimeout < 1000*TICKS_PER_MS) {
				// wait until motor has fully stopped rotating.
				// note that exactly at or shortly after turning off the motor a coin could be flying out, therfore this wait state.
			} else {
				// after 1s, give up.
				hopperResponse=HOPPER_RESPONSE_EMPTY;
				hopperState=HOPPER_STATE_COOLDOWN;
				hopperTimeout=0;
			}
			break;
			
			
		case HOPPER_STATE_DISPENSE_DETECTED_COIN: // coin was found. check that "coin present" is on for at least 5ms
			// keep motor running for a short time, then switch off
			if (!hopperCoinPresent()) {
				// error: coin impulse too short
				hopperError=HOPPER_ERR_SHORT_COIN_IMPULSE;
				return;
			}
			if (hopperTimeout < 5*TICKS_PER_MS) {
				hopperTimeout++;
			} else {
#warning adjust Td to our hopper disk color
				const uint8_t MOTOR_DELAY_TIME = 10; // "Td" delay time from datasheet: how long does the motor need to stay on *after* detecting a coin (time from sensor rising edge to motor turn-off)
				hopperPause=(MOTOR_DELAY_TIME-1-5)*TICKS_PER_MS; // "Td" delay time from datasheet, minus the 1 ms debouncing time from previous state minus the time in this state
				hopperState=HOPPER_STATE_DISPENSE_SUCCESS_FINISH;
			}
			break;
		case HOPPER_STATE_DISPENSE_SUCCESS_FINISH:
			// switch off motor, report success
			hopperMotor(0);
			hopperResponse=HOPPER_RESPONSE_SUCCESS;
			hopperPause=200*TICKS_PER_MS; // ignore coin output for 200ms
			// TODO this does not detect double payouts
			hopperState=HOPPER_STATE_COOLDOWN;
			break;
		case HOPPER_STATE_COOLDOWN:
			// cool down motor: to ensure 50% duty cycle, wait for the maximum possible motor runtime minus the wait time in HOPPER_STATE_CHECK_SENSOR_2
			if (hopperCoinPresent()) {
				hopperError=HOPPER_ERR_UNEXPECTED_COIN_AT_COOLDOWN;
				return;
			}
			if (hopperTimeout < (800-250)*TICKS_PER_MS) {
				hopperTimeout++;
			} else {
				hopperTimeout=0;
				hopperState=HOPPER_STATE_INIT;
			}
		default:
			FATAL("unknown hopper state");
	}
}