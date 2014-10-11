/*
    -----===== Arduino Web radio player ====----
    
    (c)05 October 2014 by Vassilis Serasidis
   
        Home: http://www.serasidis.gr
       email: avrsite@yahoo.gr
     Version: v1.0
   
    Hardware:
    - Any Arduino (I prefer Arduino pro mini or nano because of its small size and price).
    - ENC28J60 ethernet module  (EtherCard library is used   - Written by Jean-Claude Wippler)
    - VS1053B MP3 module/shield (VS1053B library is used     - Written by J. Coliz)
    - Nokia 5110 LCD module     (PCD8544_mod library is used - Written by Carlos Rodrigues - Modified by Vassilis Serasidis )
    
    Additional libraries:
    - TimerOne (Software watchdog timer - Originaly written by Jesse Tane, last modification Oct 2011 by Andrew Richards).
    
    Description:
    - This web radio player has 14 pre-defined web radio stations (station1, station2,...,station13, station14).
      You can add your favorite webradio stations. There is much free available flash memory.
       
    - For more web radio stations visit http://www.internet-radio.com/
      Visit the http://www.internet-radio.com/ , choose the stations with up to 64-kbps and copy the IP and the Port of the web radio.
      For example, right-click on your favorite 64-kbps webradio icon and select <save link location. Then, paste it to a text file. 
      You will see somethink like that:->  http://servers.internet-radio.com/tools/playlistgenerator/?u=http://108.163.215.90:8006/listen.pls&t=.pls
      Replace the current station "station1_IP[] = {205,164,36,153}" with the new station "station1_IP[] = {108,163,215,90}".
      Do the same to the port. Replace the "station1_Port = 80" with the new one "station1_Port = 8006".
     
     You can replace the whole 14 stations with the stations of your choice.
     
     ** This circuit doesn't use a big RAM buffer. That causes a small lag (de-synchronize) due to delay transmission of data packets from the webradio server to your webradio player.
     
     ** I will try to improve the source code for supporting 128-kbps web radio stations.
     
     
     ***** This source code and hardware are provided as is without any warranty under GNU GPL v3 licence *****
     
     Have a good listening!
*/

#include <EtherCard.h>
#include <VS1053.h>
#include <SPI.h>
#include "PCD8544_mod.h"
#include <TimerOne.h>

#define BUFFER_LENGTH 600       //Ethernet data bufer length.
#define BUFFER_LENGTH2 32       //VS1053 data buffer length
#define SW_NEXT A1              //Switch for selecting the next webradio station (Arduino pin A1)
#define SW_PREV A2              //Switch for selecting the previous webradio station (Arduino pin A2)
#define LED1    A3
#define LAST_STATION_NUMBER 14  //The last webradio station (14th)

boolean ViewStationInfo = false;
byte Ethernet::buffer[BUFFER_LENGTH];
byte MP3_buffer[BUFFER_LENGTH2];
int indexCounter = 0;
static uint32_t timer;

char radioStationNumber = 1; //Initial webradio station
boolean radioStationIsChanged = false;
boolean receivedData = true;

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[]  = { 192,168,1,200 }; //manualy sets the IP, Gateway IP and DNS in case DHCP fails.
static byte gwip[]  = { 192,168,1,1 };
static byte dnsip[] = { 255,255,255,0 };

VS1053  player(9, 6, 7, 8);  // cs_pin, dcs_pin, dreq_pin, reset_pin
PCD8544 lcd(3, 4, 5, 2, A0); // clk_pin, data_in_pin, data_select_pin, reset_pin, enable_pin

//--------------- 14 Web radio stations 64-kbps ----------------------------------------------------------------
static byte station1_IP[] = {205,164,36,153};   //BOM PSYTRANCE (1.FM TM)  64-kbps
const int   station1_Port = 80;

static byte station2_IP[] = {205,164,62,15};    //1.FM - GAIA, 64-kbps
const int   station2_Port = 10032;

static byte station3_IP[] = {109, 206, 96, 11}; //TOP FM Beograd 106,8  64-kpbs
const int   station3_Port = 80;

static byte station4_IP[] = {85,17,121,216};    //RADIO LEHOVO 971 GREECE, 64-kbps
const int   station4_Port = 8468;

static byte station5_IP[] = {85,17,121,103};    //STAR FM 88.8 Corfu Greece, 64-kbps
const int   station5_Port = 8800;

static byte station6_IP[] = {85,17,122,39};     //www.stylfm.gr laiko, 64-kbps
const int   station6_Port = 8530;

static byte station7_IP[] = {144,76,204,149};   // RADIO KARDOYLA - 64-kbps 22050 Hz
const int   station7_Port = 9940;

//static byte station7_IP[] = {192,168,1,65};   // Local server
//const int   station7_Port = 23;

static byte station8_IP[] = {198,50,101,130};   //La Hit Radio, Rock - Metal - Hard Rock, 32-kbps
const int   station8_Port = 8245;

static byte station9_IP[] = {94,23,66,155};     // *ILR CHILL & GROOVE* 64-kbps
const int   station9_Port = 8106;

static byte station10_IP[] = {205,164,62,22};    //1.FM - ABSOLUTE TRANCE (EURO) RADIO   64-kbps
const int   station10_Port = 7012;

static byte station11_IP[] = {205,164,62,13};    //1.FM - Sax4Ever   64-kbps 
const int   station11_Port = 10144;

static byte station12_IP[] = {83,170,104,91};    //Paradise Radio 106   64-kbps
const int   station12_Port = 31265;

static byte station13_IP[] = {205,164,62,13};    //Costa Del Mar - Chillout (1.FM), 64-kbps
const int   station13_Port = 10152;

static byte station14_IP[] = {46,28,48,140};     //AutoDJ, latin, cumbia, salsa, merengue, regueton, pasillos , 48-kbps
const int   station14_Port = 9998;



//=====================================================================================================
// called when the client request is complete
//=====================================================================================================
static void my_callback (byte status, word off, word len)
{
	unsigned int i;
        
        if((indexCounter < 500) && (ViewStationInfo == false))
        {
          for(i=0;i<len;i++)
          {
            Serial.write(Ethernet::buffer[off+i]); //Show the web radio channel information (name, genre, bit rate etc).
            if((indexCounter + i > 179) && (indexCounter + i < 301)) 
               lcd.write(Ethernet::buffer[off+i]);
               
            if(Ethernet::buffer[off+i] == 0x0d && 
               Ethernet::buffer[off+i+1] == 0x0a && 
               Ethernet::buffer[off+i+2] == 0x0d && 
               Ethernet::buffer[off+i+3] == 0x0a)
               {
                   ViewStationInfo = true;
                   break; //We found the index end (0x0d,0x0a,0x0d,0x0a). Do not search anymore.
               }
          }
          indexCounter += len;
        }
        else
        {
           uint8_t* data = (uint8_t *) Ethernet::buffer + off; //Get the data stream from ENC28J60 and...
           player.playChunk(data, len);                        //...send them to VS1053B
           timer = millis();                                   //Update the timeout timer.
           receivedData = true;
        }          
}

//=====================================================================================================
//
//=====================================================================================================
void setup()
{
        pinMode(SW_NEXT, INPUT);      //Make SW_NEXT pin as input
        pinMode(SW_PREV , INPUT);     //Make SW_PREV pin as input
        digitalWrite(SW_NEXT, HIGH);  //Enable internal pull-up on SW_NEXT pin
        digitalWrite(SW_PREV , HIGH); //Enable internal pull-up on SW_PREV pin
        
        pinMode(LED1, OUTPUT); //LED1 is connected to A3 pin 
        
	Serial.begin(57600);    //Start serial port with 57600 bits per seconds
	SPI.begin();            //Start Serial Peripheral Interface (SPI)
	player.begin();         //Start VS1053B
        player.setVolume(0);    //Set the volume to the maximux.
        
        lcd.begin(84, 48); //84*48 pixels lcd (Nokia 5110).
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("-= WEBRADIO PLAYER =-"));
        lcd.setCursor(0, 1);
        lcd.print(F("fw: 1.0"));
        lcd.setCursor(0, 2);
        lcd.print(F("(C) 2014 BY"));
        lcd.setCursor(0, 3);
        lcd.print(F("VASSILIS SERASIDIS"));
        lcd.setCursor(0, 5);
        lcd.print(F("WWW.SERASIDIS.GR"));
        delay(5000);
        
        
	Serial.println(F("\n--== Arduino WebRadio player ==--\n"));
        Serial.println(F("(c)2014 by Vassilis Serasidis"));
        Serial.println(F("http://www.serasidis.gr\n"));
        Serial.println(F("\nConnecting..."));

	if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0) //Initiallize ENC28J60. Chip Select pin (CS) is connected on arduino pin D10.
	  Serial.println(F("Failed to access Ethernet controller"));
	if (!ether.dhcpSetup())
	  Serial.println(F("DHCP failed"));

        ether.persistTcpConnection(true); //Keep TCP/IP connection alive. 
        ViewStationInfo = true;
        
        Timer1.initialize(5000000); // set a timer of length 5000000 microseconds (or 5 sec => the led will blink 5 times, 5 cycles of on-and-off, per second)
        Timer1.attachInterrupt( timerIsr ); // attach the service routine here
}

//=====================================================================================================
//
//=====================================================================================================
void loop()
{
   ether.packetLoop(ether.packetReceive()); //Read ENC28J60 for new incoming data packet.

   if(digitalRead(SW_NEXT) == LOW) //Check if the 'next' switch has been pressed.
   {
     if(radioStationNumber < LAST_STATION_NUMBER)
     {
       while(digitalRead(SW_NEXT) == LOW)
         delay(100);
       radioStationNumber += 1;
       radioStationIsChanged = true;
     }
   }
   
   if(digitalRead(SW_PREV)== LOW) //Check if the 'previous' switch has been pressed.
   {
       while(digitalRead(SW_PREV) == LOW)
         delay(100);
       if(radioStationNumber > 1)
       {
         radioStationNumber -= 1;
         radioStationIsChanged = true;
       }
   }
   
   if(radioStationIsChanged == true)//If 'next' or 'previous' switch has been pressed, play the selected webradio station.
   {
     switch (radioStationNumber)
     {
       case 1:
          playWebRadioStation(station1_IP, station1_Port, "01");
       break;
       
       case 2:
          playWebRadioStation(station2_IP , station2_Port, "02");
       break; 
       case 3:
          playWebRadioStation(station3_IP , station3_Port, "03");
       break; 
       case 4:
          playWebRadioStation(station4_IP , station4_Port, "04");
       break;   
       case 5:
          playWebRadioStation(station5_IP , station5_Port, "05"); 
       break;
       case 6:
          playWebRadioStation(station6_IP , station6_Port, "06"); 
       break;
       case 7:
          playWebRadioStation(station7_IP , station7_Port, "07");
       break;
       case 8:
          playWebRadioStation(station8_IP , station8_Port, "08");
       break;
       case 9:
          playWebRadioStation(station9_IP , station9_Port, "09");
       break;
       case 10:
          playWebRadioStation(station10_IP , station10_Port, "10"); 
       break;
       case 11:
          playWebRadioStation(station11_IP , station11_Port, "11");
       break;
       case 12:
          playWebRadioStation(station12_IP , station12_Port, "12");
       break;
       case 13:
          playWebRadioStation(station13_IP , station13_Port, "13");
       break;
       case 14:
          playWebRadioStation(station14_IP , station14_Port, "14");
       break;
     }
   }
  
   radioStationIsChanged = false; 
   
  if((millis() > timer + 5000)) // Timeout timer. If the song stops playing for 5 seconds re-connect to the server.
  {   
   radioStationIsChanged = true; 
   ViewStationInfo = false;
   Serial.print(F("\nre-connecting to the server...\n"));
   lcd.clear();
   lcd.setCursor(8,3);
   lcd.print(F("re-connecting to"));
   lcd.setCursor(12,4);
   lcd.print(F("the server"));
   timer = millis(); 
  }
}

//=====================================================================================================
//
//=====================================================================================================
void playWebRadioStation ( byte ip[4], const int hisPort, char* preset )
{
  //String webDir = "";
  //char webDirString[16];
  //char i,m;
 
  //webDir += String(ip[0]);
  //webDir += '.';
  //webDir += String(ip[1]);
  //webDir += '.';
  //webDir += String(ip[2]);
  //webDir += '.';
  //webDir += String(ip[3]);
  
  //m = webDir.length();
  //for(i=0;i<m;i++)
  //  webDirString[i] = webDir[i];
    
  //webDirString[m]=0;  
  Serial.print("\n\n<"); //Print the station info to the serial port
  Serial.print(preset);
  Serial.println(F("> ============================================================"));
  
  player.stopSong();
  ViewStationInfo = false;
  indexCounter = 0;
  ether.copyIp(ether.hisip, ip);
  ether.hisport = hisPort;
  ether.printIp("IP:   ", ether.myip);
  ether.printIp("GW:   ", ether.gwip);
  ether.printIp("DNS:  ", ether.dnsip);
  ether.printIp("SRV:  ", ether.hisip); 
  Serial.print("Port: ");
  Serial.println(ether.hisport);
  Serial.println();
  //ether.browseUrl(PSTR("/"), "",PSTR(""), PSTR("Icy-MetaData:1"), my_callback);
  ether.browseUrl(PSTR("/"), "",PSTR(""), PSTR(""), my_callback);
  lcd.clear();
  lcd.setCursor(0, 0); //LCD line 0
  lcd.write('<'); 
  lcd.print(preset); //Show on LCD the webradio number (1-14)
  lcd.write('>');
  lcd.write(' ');
}

//=====================================================================================================
// Custom ISR Timer Routine
//=====================================================================================================
void timerIsr()
{
    // Toggle LED
    digitalWrite( LED1, digitalRead( LED1 ) ^ 1 );
    if(receivedData == true)
      receivedData = false;
    else
    {
      radioStationIsChanged = true; 
      if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0) //Initiallize ENC28J60. Chip Select pin (CS) is connected on arduino pin D10.
	  Serial.println(F("Failed to access Ethernet controller"));
      if (!ether.dhcpSetup())
	  Serial.println(F("DHCP failed"));
      ether.persistTcpConnection(true); //Keep TCP/IP connection alive.
    }      
}
