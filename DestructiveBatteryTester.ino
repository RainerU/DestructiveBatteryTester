// The Destructive Battery Tester
// (c) Rainer Urlacher 2019
//
// for more details and usage see videos in my Youtube video channels:
// Bits and Electrons (English) in work
// Elekrobits         (German)  https://youtu.be/Gl1ISn4Dsfc
//
// hard- and software published under the MIT License
// - on github:      https://github.com/RainerU/DestructiveBatteryTester
// - and on EasyEDA: https://easyeda.com/BitsAndElectrons/destructive-batterie-tester

// including display library u8g2 by olikraus, see: https://github.com/olikraus/u8g2
#include <U8g2lib.h>

// Nokia 5110 display pins
#define displayCE  12
#define displayDIN 1
#define displayDC  0
#define displayCLK 2

// display lines
#define line1 7
#define line2 15
#define line3 23
#define line4 31
#define line5 39
#define line6 47

// display constructor, PCD8544 84X48, full framebuffer, size = 528 bytes
U8G2_PCD8544_84X48_F_4W_SW_SPI display(U8G2_R0, displayCLK, displayDIN, displayCE, displayDC);

// start points for 8 columns of one letter (for overview screen)
byte column8[] = {0, 11, 22, 33, 44, 55, 66, 77};

// digital pins for UI
#define LED 13
#define SWITCH 11

// switch names
#define modeButton 1
#define selectButton 2

// cell voltage measuring
#define digits2voltage 0.0024414 // 2.5V/1024
#define noCellVoltage 0.2 // below this value, no cell is inserted
#define numberOfCells 8 // reduce this number for less cell holders, no other changes needed
#define minStopVoltage 0.8 // lowest voltage allowed for discharge stop
#define maxStopVoltage 1.5 // highest voltage allowed for discharge stop 

// cell discharge current - measured with amp-meter right at the cell
// for best precision put your individual values in milli-amps here!
const float dischargeCurrent[] = {50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0};

// ADC input pins
#define cellVoltage0 A7
#define cellVoltage1 A6
#define cellVoltage2 A5
#define cellVoltage3 A4
#define cellVoltage4 A3
#define cellVoltage5 A2
#define cellVoltage6 A1
#define cellVoltage7 A0

// cell current source enable outputs
#define cellEnable0 3
#define cellEnable1 4
#define cellEnable2 5
#define cellEnable3 6
#define cellEnable4 7
#define cellEnable5 8
#define cellEnable6 9
#define cellEnable7 10

// states of UI state machine
#define stateSelectNext        0
#define stateChannelSelect     1
#define stateChannel           2
#define stateRunSelect         3
#define stateRun               4
#define stateModeSelect        5
#define stateAllOffSelect      6
#define stateAllOff            7
#define stateChannelOff        8
#define stateVSelect           9
#define stateVoltage          10
#define stateVSettingSelect   11
#define stateVSetting         12
#define stateVDecrease        13


// global variables
byte state = stateRun; // current state
byte channel = 0; // current channel
unsigned long runStart[] = {0, 0, 0, 0, 0, 0, 0, 0}; // start time of discharging
unsigned long runTime[]  = {0, 0, 0, 0, 0, 0, 0, 0}; // duration of discharging after completion
float channelVoltage[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // current cell voltage
unsigned long nextVoltageMeasurement = 0; // time for next voltage measurement
float stopVoltage = 0.8; // current setting of discharge stop voltage
float nextStopVoltage = stopVoltage; // new setting in the voltage change-screen

// switch specified channel off (stop discharging of cell)
void switchChannelOff(byte channel) {
	switch(channel) {
		case 0:
			pinMode(cellEnable0, OUTPUT);
			digitalWrite(cellEnable0, LOW);
			break;
		case 1:
			pinMode(cellEnable1, OUTPUT);
			digitalWrite(cellEnable1, LOW);
			break;
		case 2:
			pinMode(cellEnable2, OUTPUT);
			digitalWrite(cellEnable2, LOW);
			break;
		case 3:
			pinMode(cellEnable3, OUTPUT);
			digitalWrite(cellEnable3, LOW);
			break;
		case 4:
			pinMode(cellEnable4, OUTPUT);
			digitalWrite(cellEnable4, LOW);
			break;
		case 5:
			pinMode(cellEnable5, OUTPUT);
			digitalWrite(cellEnable5, LOW);
			break;
		case 6:
			pinMode(cellEnable6, OUTPUT);
			digitalWrite(cellEnable6, LOW);
			break;
		case 7:
			pinMode(cellEnable7, OUTPUT);
			digitalWrite(cellEnable7, LOW);
			break;
	};
};

// switch specified channel on (discharge cell)
void switchChannelOn(byte channel) {
	switch(channel) {
		case 0: pinMode(cellEnable0, INPUT); break;
		case 1: pinMode(cellEnable1, INPUT); break;
		case 2: pinMode(cellEnable2, INPUT); break;
		case 3: pinMode(cellEnable3, INPUT); break;
		case 4: pinMode(cellEnable4, INPUT); break;
		case 5: pinMode(cellEnable5, INPUT); break;
		case 6: pinMode(cellEnable6, INPUT); break;
		case 7: pinMode(cellEnable7, INPUT); break;
	};
};

// read ADC value of specified channel
int readChannelADC (byte channel) {
	switch(channel) {
		case 0: return(analogRead(cellVoltage0));
		case 1: return(analogRead(cellVoltage1));
		case 2: return(analogRead(cellVoltage2));
		case 3: return(analogRead(cellVoltage3));
		case 4: return(analogRead(cellVoltage4));
		case 5: return(analogRead(cellVoltage5));
		case 6: return(analogRead(cellVoltage6));
		case 7: return(analogRead(cellVoltage7));
		default: return(0);
	};
};

// read voltage of specified channel
float readVoltage(byte channel) {
	return (digits2voltage * (float)readChannelADC(channel));
};

// read average voltage of specified channel from 10 ADC cycles
float readAverageVoltage(byte channel) {
	float average = 0;
	readChannelADC(channel); // ignore first read
	for (byte i = 0; i < 10; i++) {
		delay(1);
		average = average + readVoltage(channel);
	};
	return(average/10.0);
};

// convert time in seconds to printable string
// t is time in seconds = millis()/1000;
// from http://forum.arduino.cc/index.php?topic=45293.msg328355#msg328355 in #10 by user robtillaart 20.11.2010 ;)
char * TimeToString(unsigned long t)
{
	static char str[12];
	long h = t / 3600;
	t = t % 3600;
	int m = t / 60;
	int s = t % 60;
	sprintf(str, "%04ld:%02d:%02d", h, m, s);
	return str;
}

// setup ...
void setup(void) {
	// initialize display
	display.begin();
	
	for (byte i = 0; i < numberOfCells; i++) switchChannelOff(i);
	
	pinMode(LED, OUTPUT);
	
	analogReference(EXTERNAL); // 2.5V at REF pin connected
};

// ... and go
void loop(void) {
	// read two push-buttons on one pin
	// with mode INPUT_PULLUP --> modeButton can be detected
	// with mode INPUT        --> modeButton OR selectButton can be detected
	byte pressed = 0;
	pinMode(SWITCH, INPUT_PULLUP); // now a low signal can only mean mode is pressed
	if (!digitalRead(SWITCH)) pressed = modeButton;
	else {
		pinMode(SWITCH, INPUT); // now a low signal could mean any button, but mode was alrady checked ...
		if (!digitalRead(SWITCH)) {
			pressed = selectButton; // ... so it must be the select button
			pinMode(SWITCH, INPUT_PULLUP); // check if the mode button was pressed just in the meantime
			if (!digitalRead(SWITCH)) pressed = modeButton;
		};
	};
	
	// UI state machine
	switch (state) {
		case stateChannelSelect: // wait for button release before going to Channel mode
			if (pressed == 0) state = stateChannel;
			break;
		case stateRunSelect: // wait for button release before going to Run mode
			if (pressed == 0) state = stateRun;
			break;
		case stateRun: // system is running according to settings, all of at start
			if (pressed == selectButton) state = stateVSelect;
			if (pressed == modeButton) state = stateAllOffSelect;
			break;
		case stateChannel: // see channel details and select channel mode
			if (pressed == selectButton) state = stateSelectNext;
			if (pressed == modeButton) state = stateModeSelect;
			break;
		case stateSelectNext: // wait for button release before going to Run/Channel mode, increase channel
			if (pressed == 0) {
				if (++channel == numberOfCells) state = stateRun;
				else state = stateChannel;
			}
			break;
		case stateModeSelect: // change mode of current channel
			if (pressed == 0) {
				if (runStart[channel] > 0) state = stateChannelOff;
				else if (runTime[channel] > 0) state = stateChannelOff;
				else {
					state = stateChannel;
					switchChannelOn(channel);
					runStart[channel] = millis();
				}
			};
			break;
		case stateChannelOff: // wait for confirmation before switching channel off
			if (pressed == selectButton) {
				state = stateChannelSelect;
				switchChannelOff(channel);
				runStart[channel] = 0;
				runTime[channel] = 0;
			};
			if (pressed == modeButton) state = stateChannelSelect;
			break;
		case stateAllOffSelect: // wait for button release before going to stateAllOff
			if (pressed == 0) state = stateAllOff;
			break;
		case stateAllOff: // wait for confirmation before switching all channels off
			if (pressed == selectButton) {
				state = stateRunSelect;
				for (byte i = 0; i < numberOfCells; i++) {
					switchChannelOff(i);
					runStart[i] = 0;
					runTime[i] = 0;
				};
			};
			if (pressed == modeButton) state = stateRunSelect;
			break;
		case stateVSelect: // wait for button release before going to stateVoltage
			if (pressed == 0) state = stateVoltage;
			break;
		case stateVoltage: // show discharging stop-voltage
			if (pressed == selectButton) {
				state = stateChannelSelect;
				channel = 0;
			};
			if (pressed == modeButton) state = stateVSettingSelect;
			break;
		case stateVSettingSelect: // wait for button release before going to stateVSetting
			if (pressed == 0) {
				state = stateVSetting;
				nextStopVoltage = stopVoltage;
			}
			break;
		case stateVSetting: // allow to set discharge off-voltage in 0.1V steps
			if (pressed == selectButton) state = stateVDecrease;
			if (pressed == modeButton) {
				state = stateVSelect;
				stopVoltage = nextStopVoltage;
			}
			break;
		case stateVDecrease: // decrease discharge stop-voltage by 0.1V
			if (pressed == 0) {
				state = stateVSetting;
				nextStopVoltage = nextStopVoltage - 0.1;
				if (nextStopVoltage < minStopVoltage - 0.001) nextStopVoltage = maxStopVoltage;
			}
			break;
	};
	
	// measure voltages (run only all 5 seconds)
	if (millis() >= nextVoltageMeasurement) {
		nextVoltageMeasurement = millis() + 5000;
		for (byte i = 0; i < numberOfCells; i++) {
			if (runStart[i] > 0) {
				channelVoltage[i] = readAverageVoltage(i);
			} else {
				switchChannelOn(i); // to test if a cell is inserted, we must switch on the current source
				if (readVoltage(i) > noCellVoltage) {
					switchChannelOff(i);
					channelVoltage[i] = readAverageVoltage(i);  // cell is inserted
				} else {
					switchChannelOff(i);
					channelVoltage[i] = 0;  // no cell is inserted
				};
			};
		};
	
		// switch channels off when cell is discharged
		for (byte i = 0; i < numberOfCells; i++) {
			if (runStart[i] == 0); // is OFF, do nothing
			else if (channelVoltage[i] <= stopVoltage) {
				runTime[i] = millis() - runStart[i];
				runStart[i] = 0;
				switchChannelOff(i);
			};
		};
	} else delay(100);
	
	// display
	display.clearBuffer();
	display.setFont(u8g2_font_6x10_tr);

	// calculate discharging time of current channel in seconds
	unsigned long timeSeconds = 0;
	if (runTime[channel] > 0) timeSeconds = runTime[channel]/1000;
	else if (runStart[channel] > 0) timeSeconds = (millis() - runStart[channel])/1000;

	// print dedicated screen for each state
	float lowestCellVoltage;
	switch(state) {
		case stateChannel: // channel screen, used when channel is discharging or not
			display.setCursor(0, line1);
			display.print("CHANNEL ");
			display.print(channel+1);
			display.print(" ");
			if (runTime[channel] > 0) display.print("DONE");
			else if (runStart[channel] > 0) display.print("RUN");
			else display.print("OFF");
			
			display.setCursor(0, line2);
			display.print("VOLT ");
			display.print(channelVoltage[channel]);
			
			display.setCursor(0, line3);
			display.print("TIME ");
			display.print(TimeToString(timeSeconds)+1);
			
			display.setCursor(0, line4);
			display.print("mAh  ");
			display.print(timeSeconds * dischargeCurrent[channel] / 3600, 0);
			
			display.setCursor(0, line5);
			if (channel < numberOfCells-1) display.print("SEL: NEXT CHAN");
			else display.print("SEL: RUN VIEW");

			display.setCursor(0, line6);
			display.print("MOD: ");
			if (runTime[channel] > 0) display.print("RESET");
			else if (runStart[channel] > 0) display.print("STOP");
			else display.print("START");
			break;
		case stateRun: // overview screen
			for (byte i = 0; i < numberOfCells; i++) {
				display.setCursor(column8[i], line1);
				display.print(i+1);
			}
			for (byte i = 0; i < numberOfCells; i++) {
				display.setCursor(column8[i], line2);
				if (channelVoltage[i] > 0) display.print("I");
			}
			for (byte i = 0; i < numberOfCells; i++) {
				display.setCursor(column8[i], line3);
				if (runTime[i] > 0);
				else if (runStart[i] > 0) display.print("R");
			}
			for (byte i = 0; i < numberOfCells; i++) {
				display.setCursor(column8[i], line4);
				if (runTime[i] > 0) display.print("D");
			}
			display.setCursor(0, line5);
			display.print("SEL: VOLTAGE");

			display.setCursor(0, line6);
			display.print("MOD: RESET ALL");
			break;
		case stateChannelOff: // reset channel - user confirmation
			display.setCursor(0, line1);
			display.print("OK TO CANCEL");

			display.setCursor(0, line2);
			display.print("CHANNEL ");
			display.print(channel);
			display.print("?");
			
			display.setCursor(0, line3);
			display.print("-> TIME RESET");

			display.setCursor(0, line5);
			display.print("SEL: RESET");
			
			display.setCursor(0, line6);
			display.print("MOD: BACK");
			break;
		case stateAllOff: // reset all channels - user confirmation
			display.setCursor(0, line1);
			display.print("OK TO CANCEL");

			display.setCursor(0, line2);
			display.print("ALL CHANNELS?");
			
			display.setCursor(0, line3);
			display.print("-> TIME RESET");

			display.setCursor(0, line5);
			display.print("SEL: RESET");
			
			display.setCursor(0, line6);
			display.print("MOD: BACK");
			break;
		case stateVoltage: // display voltage
			display.setCursor(0, line1);
			display.print("STOP VOLTAGE:");
			
			display.setCursor(0, line2);
			display.print(stopVoltage);
			display.print("V");
			
			display.setCursor(0, line3);
			display.print("LOWEST CELL-V:");
			lowestCellVoltage = 10.0;
			
			for (byte i = 0; i < numberOfCells; i++) {
				if (channelVoltage[i] < lowestCellVoltage && channelVoltage[i] > 0)
					lowestCellVoltage = channelVoltage[i];
			}
			display.setCursor(0, line4);
			if (lowestCellVoltage < 10) display.print(lowestCellVoltage);
			else display.print("--");
			display.print("V");

			display.setCursor(0, line5);
			display.print("SEL: CHANNELS");
			
			display.setCursor(0, line6);
			display.print("MOD: MODIFY V");
						break;
		case stateVSetting: // voltage changing screen
			display.setCursor(0, line1);
			display.print("STOP VOLTAGE:");
			
			display.setCursor(0, line2);
			display.print(stopVoltage);
			display.print("V -> ");
			display.print(nextStopVoltage);
			display.print("V");
			
			display.setCursor(0, line3);
			display.print("LOWEST CELL-V:");
			lowestCellVoltage = 10.0;
			
			for (byte i = 0; i < numberOfCells; i++) {
				if (channelVoltage[i] < lowestCellVoltage && channelVoltage[i] > 0)
					lowestCellVoltage = channelVoltage[i];
			}
			display.setCursor(0, line4);
			if (lowestCellVoltage < 10) display.print(lowestCellVoltage);
			else display.print("----");
			display.print("V");
			
			display.setCursor(0, line5);
			display.print("SEL: DECREASE!");

			display.setCursor(0, line6);
			display.print("MOD: SAVE");
			break;
		// following screens are displayed while a button is pressed:
		case stateSelectNext:
			display.setCursor(0, line2);
			display.print("NEXT SCREEN");
			break;
		case stateChannelSelect:
			display.setCursor(0, line2);
			display.print("TO CHANNEL");
			break;
		case stateRunSelect:
			display.setCursor(0, line2);
			display.print("RUN SCREEN");
			break;
		case stateModeSelect:
			display.setCursor(0, line2);
			display.print("CHANGE MODE");
			break;
		case stateAllOffSelect:
			display.setCursor(0, line2);
			display.print("ALL RESET");
			break;
		case stateVSelect:
			display.setCursor(0, line2);
			display.print("STOP VOLTAGE");
			break;
		case stateVSettingSelect:
			display.setCursor(0, line2);
			display.print("MODIFY");
			display.setCursor(0, line3);
			display.print("STOP VOLTAGE");
			break;
		case stateVDecrease:
			display.setCursor(0, line2);
			display.print("DECREASE");
			display.setCursor(0, line3);
			display.print("STOP VOLTAGE");
			break;
	};
	display.sendBuffer();

	// blinking LED for activity indication, only if at least one cell is discharging
	for (byte i = 0; i < numberOfCells; i++) {
		if (runStart[i] > 0) {
			digitalWrite(LED, HIGH);
			delay(80);
			digitalWrite(LED, LOW);
			break;
		};
	};
};

