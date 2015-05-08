//   Copyright 2013 Max Gaukler <development@maxgaukler.de
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

void hopperPoll(void);
uint8_t hopperStatus=0;
uint8_t hopperDoDispense=0;
uint16_t hopperDelay=0; // wie lange soll gewartet werden - in 0.1ms Einheiten - wird auf Vielfache von hopperTick gerundet
uint16_t hopperTimeout=0;
const uint8_t hopperTick=10; // wie oft wird hopperPoll() aufgerufen? Periodendauer in 0.1ms Einheiten

#define HOPPER_PORT_MOTOR PORTC
#define HOPPER_DDR_MOTOR DDRC
#define HOPPER_PIN_MOTOR PC5

#warning TODO hopper-opto-supply

#define HOPPER_INPORT_SENSOR PINC
#define HOPPER_PORT_SENSOR PORTC
#define HOPPER_PIN_SENSOR PC4

void hopperInit(void);
void hopperMotor(uint8_t);
void hopperOptoSupply(uint8_t);
uint8_t hopperSensor(void);

void hopperInit(void) {
	hopperMotor(0);
	setBit(HOPPER_DDR_MOTOR,HOPPER_PIN_MOTOR,1);
#warning disabled
	// setBit(HOPPER_DDR_OPTOSUPPLY,HOPPER_PIN_OPTOSUPPLY,1);
	// Sensor Pullup an
	setBit(HOPPER_PORT_SENSOR,HOPPER_PIN_SENSOR,1);
	hopperOptoSupply(1);
}

void hopperOptoSupply(uint8_t on) {
	#warning disabled
	// setBit(HOPPER_PORT_OPTOSUPPLY,HOPPER_PIN_OPTOSUPPLY,on);
}

void hopperMotor(uint8_t on) {
	setBit(HOPPER_PORT_MOTOR,HOPPER_PIN_MOTOR,!on);
}

uint8_t hopperSensor(void) {
	return HOPPER_INPORT_SENSOR & (1<<HOPPER_PIN_SENSOR);
}

void hopperPoll() {
	if (hopperDelay>hopperTick) {
		hopperDelay -= hopperTick;
	} else {
		hopperDelay=0;
	}
	if (hopperDelay>hopperTick/2) {
		// es wird noch gewartet, nix tun
		return;
	}
#warning TODO Fotodiode tauschen!
	switch (hopperStatus) {
		case 0:
			// Init / Optokoppler Check
			hopperStatus=1;
			hopperMotor(0);
			hopperOptoSupply(1);
			hopperDelay=30;
			break;
		case 1:
			// Init / Optokoppler Check
			// Opto an -> Sensor low
			assert(hopperSensor()==0);
			
#warning hopper opto-supply switching disabled
// 			hopperStatus=2;
//			hopperOptoSupply(0);
			hopperStatus=3;
			hopperMotor(0);

			hopperDelay=30;
			break;
		case 2:
			// Init / Optokoppler Check
			// kein Strom -> Sensor high (Pullup)
			assert(hopperSensor()==1);
			
			hopperStatus=3;
			hopperMotor(0);
			hopperOptoSupply(1);
			hopperDelay=30;
			break;

		case 3:
			// Init / Optokoppler Check
			// Opto an -> Sensor low
			assert(hopperSensor()==0);
			
			if (hopperDoDispense) {
				hopperMotor(1);
				// 3ms warten, dann muss 25ms lang keine Münze da sein
				hopperDelay=30;
				hopperTimeout=250;
				hopperStatus=5;
			} else {
				hopperStatus=4; // nichts zu tun, Idle bis was zu tun
			}
			break;
		case 4:
			// Idle
			// warte auf Befehle
			if (hopperDoDispense) {
				hopperStatus=0;
			}
			break;
		case 5:
			// Dispense Anfang - es darf keine Münze auftreten
			// TODO Sensor debounce hardware??
			if (hopperSensor()) {
				// Münze erkannt - das darf nicht sein.
				// Motornachlauf, oder Fehler und Abbruch? Unklar!
				hopperStatus=255;
				logError("COIN TOO EARLY / SENSOR GLITCH");
				pcSend(HOPPER,DISPENSE,MAYBE_FAIL);
			} else if (hopperTimeout>hopperTick) {
				hopperTimeout -= hopperTick;
			} else {
				logDebug("coin not too early, waiting");
				hopperStatus=6;
				hopperTimeout=50000;
			}
			break;
		case 6:
			// Dispense Mitte - Münze soll auftreten
			if (hopperSensor()) {
				// Münze erkannt - gut!
				// Motornachlauf 15ms
				hopperDelay=150;
				hopperStatus=7;
				pcSend(HOPPER,DISPENSE,1);
				hopperDoDispense--;
				logDebug("successfully dispensed Coin");
			} else if (hopperTimeout>hopperTick) {
				hopperTimeout -= hopperTick;
			} else {
				logWarning("could not dispense coin");
				pcSend(HOPPER,FAIL,6);
				hopperStatus=255;
				hopperTimeout=0;
			}
			break;
		case 7:
			// Abbremsen, warten, Motor abkühlen lassen. Dann gehts von vorne los.
			hopperMotor(0);
			hopperStatus=0;
			hopperDelay=15000;
			break;
		case 255:
			// deaktiviert weil kaputt.
			hopperMotor(0);
			break;
		default:
			assert(0);
	}
}