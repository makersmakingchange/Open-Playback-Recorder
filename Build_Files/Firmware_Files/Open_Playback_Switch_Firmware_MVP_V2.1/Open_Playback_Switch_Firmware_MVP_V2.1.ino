
/*
  December 19, 2022
  Erik Steinthorson


  The Open Playback Button is a device capable of recording voice messages and storing them for future playback.   
  This MVP code will showcase the basic functionality of the device. Messages can be recorded after entering record 
  mode by holding the record button for 2 seconds. In record mode, multiple messages can be recorded to a micro SD 
  card by pressing and holding the play button before pressing the record button again to exit record mode. The stored
  messages can then be played back with each press of the play button. An external button can be plugged into a 3.5 mm
  mono jack to act as an accessible play button.  

  July 25, 2023
  Brad Wellington

  Updating code to work with the new microcontroller and include the 3 levels or recordings

  Components:

  Play button
  Record button
  Volume potentiometer with on/off switch
  play/rec leds
  3.5 mono jack

  Microphone and Amp
    MAX9814        Arduino Uno
      GND -----------> GND
      VDD -----------> 5V 
      Out -----------> A0

  Micro SD Breakout Board
    SD Module       Arduino Uno
      5V  -----------> 5V
      GND -----------> GND
      CLK -----------> SCK
      DO  -----------> MI
      DI  -----------> MO
      CS  -----------> SS

  Audio Amp: PAM8302A
    Amp Module       Arduino Uno
      GND -----------> GND
      Vin -----------> 5V
      SD  -----------> D11
      A-  -----------> GND 
      A+  -----------> D9 (Through Vol Pot)
      +   -----------> Speaker postive
      -   -----------> Speaker negitive
 
*/

#include <TMRpcm.h>
#include <SD.h>
#include <SPI.h>

#define DEBUG


//-----------------------------------------------------------------------------------//
//
//***GLOBAL VARIABLE DECLARATION***//
//
//-----------------------------------------------------------------------------------//

TMRpcm audio;
File myFile;

//Additional Arduino connections
const int mic {A0};
const int rec_led {A1};
const int play_led {A2};
const int switch_jack {6};
const int rec_button {5};
const int play_button {4};
const int level_button {3};
//const int power_switch {10};
const int speaker_shutdown {8};
const int level_3 {A5};
const int level_2 {A4};
const int level_1 {A3};

//File properties
int file_number =0;
int file_level {1};
int old_level = {1};
int current_message {0};
const int sample_rate {16000};
const String file_extension {".wav"}; 
int old_file_level {1};

//-----------------------------------------------------------------------------------//
//***GET MESSAGE COUNT FUNCTION***///
//
//  This function searches the SD card and counts the total number of saved messages
//
//  Parameters  : Void
//
//  Return:     : int
//
//-----------------------------------------------------------------------------------//

int get_message_count(){
  file_number=0;
  level_check();
  int count {1};
  char file [50];
  String file_name {"rec_"};
    file_name += file_level;
    file_name += "_";
    file_name += count;
    file_name += file_extension; 
    file_name.toCharArray(file,50);
  while(SD.exists(file)){
    file_number++;
    count++;
    String file_name {"rec_"};
    file_name += file_level;
    file_name += "_";
    file_name += count;
    file_name += file_extension; 
    file_name.toCharArray(file,50);
  }
  
  #ifdef DEBUG
  Serial.print("Number of Messages: ");
  Serial.println(count);
  #endif
  
  return count;
}

void level_int(){
  file_level++;
  if(file_level>3){
    file_level=1;
  }
}

//-----------------------------------------------------------------------------------//
//***COUNT MESSAGEs FUNCTION***///
//
//  Description : This function counts the number of recordings on the current level
//
//  Parameters  : int representing the current recording level
//
//  Return:     : int representing the number of messages on the current level
//-----------------------------------------------------------------------------------//

//Function that record a message and saves it to the list of messages on the SD card.
int count_messages(int file_level){
  int numFiles = 0;
  int recNumber = 1;
  while(true){
    String file_name {"rec_"};
    file_name += file_level;
    file_name += "_";
    file_name += recNumber;
    file_name += file_extension; 
    
    //convert file name to char as defined in TMRpcm library
    char file [50];
    file_name.toCharArray(file,50);
    if(!(SD.exists(file))){
       break;
       }
    recNumber++;
    numFiles++;
  }
  Serial.print("Num Files: ");
  Serial.println(numFiles);
  return numFiles;
}

//-----------------------------------------------------------------------------------//
//***RECORD MODE CHECK FUNCTION***///
//
//  Description : This function is used to determine if the record mode should be engaged: rec button has been held for 2 seconds
//
//  Parameters  : void
//
//  Return      : bool
//
//-----------------------------------------------------------------------------------//

bool record_mode_check(){
  int timer {0};
  level_check();
  while(digitalRead(rec_button) == LOW){
    delay(100);
    timer++;

    //given rec has been held for 2 seconds, proceed to record mode and clear previous messages
    if(timer > 20){
      digitalWrite(rec_led, HIGH);
      
      #ifdef DEBUG
      Serial.println("RECORD MODE");
      #endif
      
      while(digitalRead(rec_button) == LOW){
        //wait until tell rec button is released to proceed
        
      }
      Serial.println("Getting number of files");
      file_number = count_messages(file_level);
      Serial.println("Files Counted");
      //DELETE STORED MESSAGES
      for(int i =1; i <= file_number; i++){
        String file_name {"rec_"};
        file_name += file_level;
        file_name += "_";
        file_name += i;
        file_name += file_extension; 

        //convert file name to char as defined in TMRpcm library
        char file [50];
        file_name.toCharArray(file,50);

        
        if(SD.exists(file)){
          #ifdef DEBUG
          Serial.print("Removing old message: ");
          Serial.println(file);
          #endif
          SD.remove(file_name);
          #ifdef DEBUG
          Serial.println("File removed");
          #endif
        }
      }

      //Reset file properties
      current_message = 1;
      file_number = 0;
      return true;
    }
  }
  return false;
}




//-----------------------------------------------------------------------------------//
//***PLAY MESSAGE FUNCTION***///
//
//  Description : This function will play the next message in queue
//
//  Parameters  : void
//
//  Return:     : void
//
//-----------------------------------------------------------------------------------//
 
void play_message(){
  old_file_level=file_level;
  level_check();
  if(file_level!=old_file_level){
    current_message=1;
  }
  get_message_count();
  String file_name {"rec_"};
  file_name += file_level;
  file_name += "_";
  file_name += current_message;
  file_name += file_extension; 

  //convert file name to char as defined in TMRpcm library
  char file [50];
  file_name.toCharArray(file,50);

  #ifdef DEBUG  
  Serial.print("Playing message number ");
  Serial.println(current_message);
  #endif
  
  //turn on speaker and play message
  digitalWrite(speaker_shutdown, HIGH);
  audio.play(file);

  //blink playback led until message finishes
  while(audio.isPlaying()){
    digitalWrite(play_led, LOW);
    delay(500);
    digitalWrite(play_led, HIGH);
    delay(500);
  }

  //turn speaker off
  audio.stopPlayback();
  digitalWrite(speaker_shutdown, LOW);

  //prepare next message in queue. Start at beginning if at end of queue
  current_message++;
  #ifdef DEBUG
  Serial.print("Current Message: ");
  Serial.println(current_message);
  #endif
  if(current_message > file_number){
    current_message = 1;
  }
  return;
}

//-----------------------------------------------------------------------------------//
//***LEVEL CHECK FUNCTION***///
//
//  Description : This function checks the voltage of the switch to determine the message level
//
//  Parameters  : void
//
//  Return:     : void
//-----------------------------------------------------------------------------------//

void level_check(){
  delay(75);
  if(file_level!=old_level){
  digitalWrite(level_1,LOW);
  digitalWrite(level_2,LOW);
  digitalWrite(level_3,LOW);
  #ifdef DEBUG
  Serial.print("File Level: ");
  Serial.println(file_level);
  #endif
  if(file_level==1){
    digitalWrite(level_1,HIGH);
  }
  else if(file_level==2){
    digitalWrite(level_2,HIGH);
  }
  else{
    digitalWrite(level_3,HIGH);
  }
  old_level=file_level;
  }
}



//-----------------------------------------------------------------------------------//
//***RECORD MESSAGE FUNCTION***///
//
//  Description : This function records a message and saves it to the queue of messages saved to the SD card
//
//  Parameters  : void
//
//  Return:     : void
//-----------------------------------------------------------------------------------//

//Function that record a message and saves it to the list of messages on the SD card.
void record_message(){
  level_check();
  get_message_count();
  file_number++;
  String file_name {"rec_"};
  file_name += file_level;
  file_name += "_";
  file_name += file_number;
  file_name += file_extension; 

  //convert file name to char as defined in TMRpcm library
  char file [50];
  file_name.toCharArray(file,50);
  
  //Start the recording
  #ifdef DEBUG
  Serial.println("Recording Started");
  #endif
  
  audio.startRecording(file, sample_rate, mic);
  
  //blink rec led until playback button is released, ending the recording
  while(digitalRead(play_button) == LOW){
    digitalWrite(rec_led, LOW);
    delay(500);
    digitalWrite(rec_led, HIGH);
    delay(500);
  }

  //Stop the recording
  audio.stopRecording(file);
  
  #ifdef DEBUG
  Serial.println("Recording Stopped");
  
  //check if audio file was saved to the SD card
  if(SD.exists(file)){
    Serial.print(file);
    Serial.println(" has been saved to the SD card");
  }else{
    Serial.print("Error: ");
    Serial.print(file);
    Serial.println(" wasn't saved to SD card"); 
  }
  #endif

  
  
  return;
}




//-----------------------------------------------------------------------------------//
//***MICROCONTROLLER AND MODULE CONFIGURATION***///
//
//  Description : This function initializes variables, pins, and libraries, running once at power up or reset
//
//  Parameters  : void
//
//  Return:     : void
//-----------------------------------------------------------------------------------//

void setup() {

  Serial.begin(9600);

  #ifdef DEBUG
  while (!Serial) {
    // wait for serial port to connect
  }
  #endif
  //Define pins
  pinMode(rec_led, OUTPUT);
  pinMode(play_led, OUTPUT);
  pinMode(level_1, OUTPUT);
  pinMode(level_2, OUTPUT);
  pinMode(level_3, OUTPUT);
  pinMode(rec_button, INPUT_PULLUP);
  pinMode(play_button, INPUT_PULLUP);
  pinMode(level_button, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(level_button),level_int,FALLING);
  //pinMode(power_switch, INPUT_PULLUP);
  pinMode(speaker_shutdown, OUTPUT);
  pinMode(switch_jack,INPUT_PULLUP);
  //audio.CSPin = 10;
  audio.speakerPin = 9;
  audio.volume(5);
  audio.quality(1);

  //Ensure access to Micro SD Card

  #ifdef DEBUG
  Serial.println("Initializing SD card...");
  #endif
  
  if (!SD.begin(10)) {
    #ifdef DEBUG
    Serial.println("Initialization failed!");
    #endif
    
    while (1){
      digitalWrite(rec_led, HIGH);
      digitalWrite(play_led, HIGH);
      delay(500);
      digitalWrite(rec_led, LOW);
      digitalWrite(play_led, LOW);
      delay(500);
    }
    
  }
  level_check();
  digitalWrite(level_1,HIGH);
  #ifdef DEBUG
  Serial.println("Initialization done.");
  #endif
  /*
  digitalWrite(speaker_shutdown, HIGH);
  //Serial.println("Start Sound");
  char startup [50] = "startup.wav";
  audio.play(startup);
  while(audio.isPlaying()){
    
  }
  digitalWrite(speaker_shutdown, LOW);
  */
}




//-----------------------------------------------------------------------------------//
//***MAIN LOOP***///
//
//  Description : This function loops continuously, checking for changes
//
//  Parameters  : void
//
//  Return:     : void
//-----------------------------------------------------------------------------------//

void loop() {

/*
//POWER SWITCH
if(digitalRead(power_switch) == !LOW){
   digitalWrite(rec_led, LOW);
   digitalWrite(play_led, LOW);

   #ifdef DEBUG
   Serial.println("OFF");
   Serial.print ("message count is ");
   Serial.println(get_message_count());
   #endif
   
   while(digitalRead(power_switch) == !LOW){
    //wait untill turned on
   }
}else{
  digitalWrite(play_led, HIGH);

}
*/
  

//MESSAGE RECORDING   
  bool record_mode = record_mode_check(); 
  level_check();
  while(record_mode){
    digitalWrite(play_led, LOW);
    digitalWrite(rec_led, HIGH);  

    //Record a message when playback button is held
    if(digitalRead(play_button) == LOW){
      record_message();
    }

    //Exit record mode after rec_button is pressed
    if(digitalRead(rec_button) == LOW){
      record_mode = false;
      digitalWrite(rec_led, LOW);
      
      #ifdef DEBUG
      Serial.println("END OF RECORD MODE");
      #endif
    }
  }

  if(digitalRead(level_button)==LOW){
    file_level++;
  if(file_level>3){
    file_level=1;
  }
  delay(250);
  }
//MESSAGE PLAYBACK
  if((digitalRead(play_button) == LOW)||(digitalRead(switch_jack) == LOW)){
    digitalWrite(play_led, HIGH);
    
    //Play message
    play_message();
   
    //wait until button is lifted
    while(digitalRead(play_button) == LOW){
    }
    digitalWrite(play_led, LOW);
  }  
  //turn off playback light
  

//END OF LOOP
}
