
#include <avr/io.h>
#include <avr/interrupt.h>
#include "pinDefines.h"
#include "i2c_master.h"
#include <util/delay.h> 

// -------- Defines -------- //

#define TEA5767_ADDRESS 0b11000000

#define setBit(sfr, bit)     (_SFR_BYTE(sfr) |= (1 << bit))
#define clearBit(sfr, bit)   (_SFR_BYTE(sfr) &= ~(1 << bit))
#define toggleBit(sfr, bit)  (_SFR_BYTE(sfr) ^= (1 << bit))

#define bit_is_clear(sfr, bit)   (!(_SFR_BYTE(sfr) & _BV(bit)))

double frequency = 99.9;
uint8_t bytes[5];
int mute = 1;

// -------- Functions --------- //

void writeToTEA5767 () {
	i2c_start(TEA5767_ADDRESS);
	i2c_transmit(TEA5767_ADDRESS, bytes, 5);
	i2c_stop();
}

unsigned int calcPLL(double freq) {
	//High side injection calculation
	unsigned int base = 4*(freq*1000000+225000)/32768;
	return base;
}

//Translate 8 bits over to get first 6 bits
unsigned char byteOne (unsigned int x) {
	return (x >> 8);
}

//Last 8 bits that you translated in byteOne
unsigned char byteTwo (unsigned int x) {
	unsigned char c = 0xFF;
	return x & c;
}

void changeFrequency () {
	bytes[0] = byteOne(calcPLL(frequency));
	bytes[1] = byteTwo(calcPLL(frequency));
	writeToTEA5767();
}

void muteRadio () {
	if(mute) {
		bytes[0] = bytes[0] | 0x80;
		mute = 0;
	}
	else {
		bytes[0] = byteOne(calcPLL(frequency));
		mute = 1;
	}
	writeToTEA5767();
}

ISR(PCINT2_vect) {
	if(bit_is_clear(BUTTON_PIN, BUTTON)) {
		frequency -= 0.2;
		if(frequency < 88.3) {
			frequency = 88.3;
		}
		changeFrequency();
	}

	if(bit_is_clear(BUTTON_PIN, BUTTON2)) {
		frequency += 0.2;
		if(frequency > 107.9) {
			frequency = 107.9;
		}
		changeFrequency();
		
	}

	if(bit_is_clear(BUTTON_PIN, PD1)) {
		muteRadio();
	}

	toggleBit(LED_PORT, PB0);
	_delay_ms(200);
}

void setPullups(void) {
	BUTTON_PORT |= (1 << BUTTON);
	BUTTON_PORT |= (1 << BUTTON2);
	BUTTON_PORT |= (1 << PD1);
}

void initInterrupt(void) {
	PCICR |= (1 << PCIE2);
	PCMSK2 |= ( (1 << BUTTON) | (1 << BUTTON2) | ( 1 << PD1) );
	sei();
}
// -------------- MAIN --------------------------------//

int main(void) {

	bytes[0] = byteOne(calcPLL(frequency)); //0b00101111;
	bytes[1] = byteTwo(calcPLL(frequency)); //0b10111110;
	bytes[2] = 0xB2;
	bytes[3] = 0x10;
	bytes[4] = 0X00;

	i2c_init();
	changeFrequency();
	muteRadio();
	
	setPullups();
	initInterrupt(); 

	while(1) {
		
	}
	
	return 0;
}