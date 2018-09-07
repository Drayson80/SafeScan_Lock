
/* 
Gun Safe Bio Firmware
=============================================================================================================

Hardware:
---------
1x LED red with 220 ohm resistor 
1x LED multi-color Kingbright L-154A4SURKQBDZGC (red/green/blue) with 220 ohm(r/g) and 91ohm (b) resistors
2x Push Button (1x internal, 1x external)
1x Fingerprint Scanner - 5V TTL GT-511C1R (https://www.sparkfun.com/products/13007) (Pinout: left=>right: 1 = TX, 2 = Rx, 3 = Gnd, 4 = VIn)
	It uses 3.3v logic instead of the Arduino�s 5v. Use a voltage divider Arduino tx � R1(560ohm) � FPS rx � R2(1kohm) � ground (see diagram)
1x Fingerprint Scanner - 5V TTL GT-511C1R (https://www.sparkfun.com/products/13007) (Pinout: left=>right: 1 = TX (blk), 2 = Rx, 3 = Gnd, 4 = VIn)
	It uses 3.3v logic instead of the Arduino's 5v. Use a voltage divider Arduino => tx R1(560ohm) => FPS rx => R2(1kohm) => ground (see diagram)
	https://learn.sparkfun.com/tutorials/fingerprint-scanner-hookup-guide
1x Servo
1x 4xAA Battery block

WakeOnPower/sleep HW:
- 47myF capacitor
- 1x PNP transistor BC558
- 1x NPN transistor BC550
- 1x 1 kOhm resistor
- 1x 100 kOhm resistor
- 1x 22 kOhm resistor

Behavior:
---------
similar to https://www.youtube.com/watch?v=XIz2tcNXr8w

  Storing Fingerprints:
    - Activate enroll via push button "enroll_button"
    - LED green flashes 4 times
    - Fingerprint needs to get scanned 3 times
       #1
       - fingerprint LED starts glowing
       - if first scan is ok
            - fingerprint LED ends glowing
            - green LED flashes flashes 1 times
       - else red LED flashes 1 times
       #2
       - fingerprint LED starts glowing
       - if second scan is ok
            - fingerprint LED ends glowing
            - green LED flashes flashes 2 times
       - else red LED flashes 2 times fast
       #3
       - fingerprint LED starts glowing
       - if third scan is ok
           - fingerprint LED ends glowing
           - green LED flashes flashes 3 times
       - else red LED flashes 3 times fast
    - if scanning sequence is ok
       - LED green flashes 2 times long
       (- Arduino enters sleep mode)
    - else red LED flashes 2 times long
  
  Delete ALL stored Fingerprints:
      - Activate delete sequence via push button "enroll_button" held for 5 seconds
      - LED red flashes 10 times
      - confirm delete sequence via push button "enroll_button"
      - LED red flashes 5 times
      - Run del routine, pause for 3 seconds
      - LED green flashes 10 times to confirm delete sequence

  Operation:
    - Push button "activate_button" wakes up Arduino and initiate opening sequence
    - fingerprint LED starts glowing
    - Scan finger
      - Compare fingerprint with database
        - if fingerprint is ok
            - flash green LED 3 times
            - move servo to open lock
            - move servo to initial position 2 second after activation
			- activate internal LED light for 10 sec.
        - else (fingerprint is nok) 
            - flash red LED 5 times fast (beepbeepbeepbeepbeep)
            - block operation for 20 seconds
    - set Arduino in sleep mode

  NiceToHave Functions:
    - battery lifetime check and feedback if voltage is too low (e.g. double flash every minute, double beep every minute)

  
Coding assistance:
------------------
FPS:
http://wordpress.hawleyhosting.com/ramblings/?p=375
http://www.homautomation.org/2014/10/11/playing-with-finger-print-scanner-fps-on-arduino/

hardware-triggered "sleep mode":
http://www.clauduino.i-networx.de/environment/en012.html

Hold Button:
http://playground.arduino.cc/Code/HoldButton

Blink without using delay()
http://playground.arduino.cc/Learning/BlinkWithoutDelayDe
https://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
https://learn.adafruit.com/multi-tasking-the-arduino-part-1/now-for-two-at-once

Servo:
http://funduino.de/index.php/3-programmieren/nr-12-servo-ansteuern
https://www.karlssonrobotics.com/tutorials/arduino/workaround-for-servo-jitter-on-arduino/

=============================================================================================================
*/

//===========================================================================
//=================DEFINITIONS===============================================
//===========================================================================

//Libraries to include
   #include <Servo.h>                         //load servo library
   #include "FPS_GT511C1R.h"                  //load FPS_GT511C3 library
   #include <SoftwareSerial.h>                // use FPS_GT511C3.h
 
// == constants won't change. They're used here to set pin numbers:
    const int enroll_button_Pin = 2;            // define PIN for button used for start enroll sequence

    const int fps_pin_TX = 4;                   // define TX PIN for fingerprint sensor
    const int fps_pin_RX = 5;                   // define RX PIN for fingerprint sensor

    const int relaisPin =  7;                   // define PIN number of relais circuit

    const int light_ledPin = 8;                 // define PIN number of the internal lighting LED

    const int servo_Pin =  9;                   // define PWM PIN for servo
    //const int servo_pos_opened = 105;           // define servo position in degree (0�-180�)
    const int servo_pos_opened = 1000;          // define servo position in Microseconds (0�= 1000, 180�=2000)
    //const int servo_pos_closed = 15;            // define servo position in degree (0�-180�)
    const int servo_pos_closed = 2000;          // define servo position in Microseconds (0�= 1000, 180�=2000)

    const int grn_ledPin =  10;                 // define PIN number of the green LED
    const int red_ledPin =  11;                 // define PIN number of the red LED
    const int blu_ledPin =  12;                 // define PIN number of the red LED 

    const long interval = 1000;                 // interval at which to blink (milliseconds)

// == other definitions
    Servo lock_operator;                        // set servo
    FPS_GT511C1R fps(fps_pin_TX, fps_pin_RX);   // set fps sensor

    int current_enroll;                         // Current state of the button
    int current_activate;                       // Current state of the button

// == variables will change:
    boolean lock_open=true;

    int enroll_button_PinState=LOW;
//    int activate_button_PinState=LOW;

    boolean grn_ledState = LOW;                  // ledState used to set the LED
    boolean red_ledState = LOW;                  // ledState used to set the LED
    boolean blu_ledState = LOW;                  // ledState used to set the LED

    long millis_held_enroll;                     // How long the button was held (milliseconds)
    long secs_held_enroll;                       // How long the button was held (seconds)
    long prev_secs_held_enroll;                  // How long the button was held in the previous check
    byte previous_enroll = HIGH;
    unsigned long firstTime_enroll;              // how long since the button was first pressed

    long millis_held_activate;                   // How long the button was held (milliseconds)
    long secs_held_activate;                     // How long the button was held (seconds)
    long prev_secs_held_activate;                // How long the button was held in the previous check
    byte previous_activate = HIGH;
    unsigned long firstTime_activate;            // how long since the button was first pressed



//=============================Routines======================================

//===Routine for servo move to open and close the lock
    void open_close()
    {
      lock_operator.write(servo_pos_opened);                 // move to "opened" position to release lock - in �
    //lock_operator.writeMicroseconds(servo_pos_opened);     // move to "opened" position to release lock - in microseconds
      fps.SetLED(false);
      delay(2000);                               // keep the position for 2 seconds
      lock_operator.write(servo_pos_closed);                 // move back to "closed" position - in �
    //lock_operator.writeMicroseconds(servo_pos_opened);     // move to "opened" position to release lock - in microseconds
      ledblink(1,10000,10,light_ledPin);                     // turn on internal lighting at light_ledPin
      delay(10000);                                          // wait 10 seconds to make sure everything is clear
      digitalWrite(relaisPin, LOW);                          // cut power on arduino to put it "sleep"
    }

//===Simple helper function to blink an led in various patterns
    void ledblink(int times, int lengthms_on, int lengthms_off, int pinnum)
    {
      for (int x=0; x<times; x++) {
        digitalWrite(pinnum, HIGH);
        delay (lengthms_on);
        digitalWrite(pinnum, LOW);
        delay(lengthms_off);
      }
    }
    
//===Routine for enroll fingerprints
    void enroll()
{
      fps.Open(); 					                                        		// start scanner
      ledblink(4,100,100,blu_ledPin);                                   // flash grn_ledPin 4 times
      delay(100);
  
  	// Enroll test
	// find open enroll id
	int enrollid = 0;
	bool usedid = true;
	while (usedid == true)
	{
		usedid = fps.CheckEnrolled(enrollid);
		if (usedid==true) enrollid++;
	}
	fps.EnrollStart(enrollid);

	// enroll
	Serial.print("Press finger to Enroll #");
	Serial.println(enrollid);
        fps.SetLED(true); 						                                      // scanner LED on
	while(fps.IsPressFinger() == false) delay(100);
	bool bret = fps.CaptureFinger(true);
	int iret = 0;
	if (bret != false)
	{
		Serial.println("Remove finger");
		      fps.SetLED(false);                                                // scanner LED off
          ledblink(1,100,10,blu_ledPin);                                  // flash grn_ledPin 1 times
		fps.Enroll1(); 
		while(fps.IsPressFinger() == true) delay(100);
		Serial.println("Press same finger again");
		    fps.SetLED(true); 			                                            // scanner LED on
		while(fps.IsPressFinger() == false) delay(100);
		bret = fps.CaptureFinger(true);
		if (bret != false)
		{
			Serial.println("Remove finger");
  			      fps.SetLED(false); 			                                      // scanner LED off
              ledblink(2,100,10,blu_ledPin);                              // flash grn_ledPin 2 times
			fps.Enroll2();
			while(fps.IsPressFinger() == true) delay(100);
			Serial.println("Press same finger yet again");
  			    fps.SetLED(true); 				                                      // scanner LED on
			while(fps.IsPressFinger() == false) delay(100);
			bret = fps.CaptureFinger(true);
			if (bret != false)
			{
				Serial.println("Remove finger");
					fps.SetLED(false); 		                                            // scanner LED off
          ledblink(3,100,10,blu_ledPin);                                 // flash grn_ledPin 3 times
				iret = fps.Enroll3();
				if (iret == 0)
				{
					Serial.println("Enrolling Successfull");
                                        ledblink(2,500,10,grn_ledPin);      // flash grn_ledPin 2 times long
				}
				else
				{
					Serial.print("Enrolling Failed with error code:");
					Serial.println(iret);
          ledblink(2,500,20,red_ledPin);      // flash red_ledPin 2 times long
				}
			}
			else Serial.println("Failed to capture third finger");
                        ledblink(3,200,20,red_ledPin);                      // flash red_ledPin 3 times
		}
		else Serial.println("Failed to capture second finger");
                ledblink(2,200,20,red_ledPin);                              // flash red_ledPin 2 times
	}
	else Serial.println("Failed to capture first finger");
  
  ledblink(1,200,20,red_ledPin);                                      // flash red_ledPin 1 times
  fps.SetLED(false);
  delay(5000);                                                      // wait 5 seconds to make sure everything is clear
  digitalWrite(relaisPin, LOW);                                      // cut power on arduino to put it "sleep"
}
    
//===Routine for delete all stored fingerprints
    void del_all()
    {
          ledblink(10,100,100,red_ledPin);                                  // flash red_ledPin 10 times
          delay(1000);
      digitalRead(enroll_button_Pin);                                       // read enroll_button
      if (enroll_button_Pin == HIGH)                                        // triggered by enroll_button to confirm delete sequence
          ledblink(5,200,100,blu_ledPin);                                   // flash blu_ledPin 5 times 
          fps.Open();
          fps.SetLED(false);
          fps.DeleteAll();		  	                                          // Delete all stored finger print
          // fps.DeleteId(id_to_remove)	                                    // if you want to remove a given id use - e.g. for ID1: fps.DeleteID(1);
          delay(3000);                                                      // wait 3 seconds to make sure everything is clear
          fps.Close();
          ledblink(10,200,100,red_ledPin);                                  // flash red_ledPin flashes 10 times to confirm operation 
          delay(5000);                                                      // wait 5 seconds to make sure everything is clear
          digitalWrite(relaisPin, LOW);                                      // cut power on arduino to put it "sleep"
    }

//===Routine compare a finger print with stored ones
    void compare_fp()
    {
      fps.Open();							// start scanner
      fps.SetLED(true); 						// scanner LED on
      delay(100);
	if (fps.IsPressFinger())					// if a finger is on the sensor
	{
		fps.CaptureFinger(false);				//capture the finger print
		int id = fps.Identify1_N();				//get the id
		if (id >= 0 && id <20)						//check with database - if Id > 20 is a not recognized one. maximun finger print stored in 20.
		{
			Serial.print("Verified ID:"); 			// finger print recognized
			Serial.println(id); 		         	// display the id
			//...
			// add you code here for the condition access allowed
                        ledblink(1,500,200,grn_ledPin);                 // flash grn_ledPin 1 times
                        open_close();                                   // call open_close routine to open lock
			//...
		}
		else
		{
			Serial.println("Finger not found");		// serial message that finger print not recognized
                        //...
			// add you code here for the condition access disallowed
                        ledblink(2,200,200,red_ledPin);                 // flash red_ledPin 2 times
                fps.SetLED(false);
                        delay(10000);                                   // wait 10 seconds to block new unlock trial
                digitalWrite(relaisPin, LOW);                   // cut power on arduino to put it "sleep"
			//...
		}
        }
	else
	{
		Serial.println("Please press finger");	               // wait for finger
                ledblink(2,500,200,grn_ledPin);                        // flash grn_ledPin 1 times
	}
    }

//===========================================================================
//=================SETUP - code to run once==================================
//===========================================================================

void setup() 
{
  // initialize relais:
  pinMode(relaisPin, OUTPUT);
  digitalWrite(relaisPin, HIGH);
  
  // initialize buttons:
  pinMode(enroll_button_Pin, INPUT);              // initialize the button pins as a input
  digitalWrite(enroll_button_Pin, HIGH);          // activate internal 20k pull-up - normally TRUE, when pushed, FALSE
  
  // initialize LEDs:
  pinMode(grn_ledPin, OUTPUT);                    // initialize the led pins as a output
  pinMode(red_ledPin, OUTPUT);                    // initialize the led pins as a output
  pinMode(blu_ledPin, OUTPUT);                    // initialize the led pins as a output
  pinMode(light_ledPin, OUTPUT);                  // initialize the led pins as a output
  
  // initialize servo
  lock_operator.attach(servo_Pin, 1000, 2000);    // assign servo to PIN
  lock_operator.write(servo_pos_closed);          // set servo to closed position
  
  // initialize serial communication:
  Serial.begin(9600);
  delay(100);
 
}

//===========================================================================
//=================LOOP - main code to run repeatedly========================
//===========================================================================

void loop() 
{
  if (millis() > 30000)
  {
    fps.SetLED(true);digitalWrite(relaisPin, LOW);               // cut power on arduino to put it "sleep" if no action for more than 30 sec
    }
      
//========ACTIVATE functions on startup
        compare_fp();                                            // call compare function
  
//========ENROLL BUTTON functions
//=== - enter enroll mode when button is pushed 2 seconds
//=== - enter delete mode when button is pushed 6 seconds

  current_enroll = digitalRead(enroll_button_Pin);
  // if the button state changes to pressed, remember the start time 
  if (current_enroll == LOW && previous_enroll == HIGH && (millis() - firstTime_enroll) > 200) {
    firstTime_enroll = millis();
  }
  millis_held_enroll = (millis() - firstTime_enroll);
  secs_held_enroll = millis_held_enroll / 1000;

  // This if statement is a basic debouncing tool, the button must be pushed for at least
  // 100 milliseconds in a row for it to be considered as a push.
  if (millis_held_enroll > 50) {

    if (current_enroll == LOW && secs_held_enroll > prev_secs_held_enroll) {
      ledblink(1, 50, 50, grn_ledPin); // Each second the button is held blink the indicator led
    }
    // check if the button was released since we last checked
    if (current_enroll == HIGH && previous_enroll == LOW) {
      // HERE ADD VARIOUS ACTIONS AND TIMES
      // ....
      
      // Button pressed for less than 1 second, one long LED blink
      //if (secs_held_enroll <= 0) {
      //  ledblink(3,200,red_ledPin);
      //}

      // If the button was held for longer than 6 seconds initiate routine for deleting all stored fingerprints
      if (secs_held_enroll >= 6) {
         del_all();                                              //call del_all function
      }

      // Button held for less than 2 seconds, initiate enroll routine
      if (secs_held_enroll <= 2) {
         enroll();                                               // call enroll function
      }
      // ....
    }
  }

  previous_enroll = current_enroll;
  prev_secs_held_enroll = secs_held_enroll;
  
}
//===========================================================================
//================= E N D   of   S K E T C H ================================
//===========================================================================
