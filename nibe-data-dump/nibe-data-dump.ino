
# Arduino sketch for collecting data msgs from Nibe Fighter 113x/1150 series.
#
# Mats Johansson, 2017


#include "WiFiEsp.h"

# parameters for Wifi and HTTP REST end-point

char ssid[] = "<SSID>";           // network SSID 
char pass[] = "<WPA Password>";   // network WPA password
int status = WL_IDLE_STATUS;      // Wifi radio status
char server[] = "192.168.1.90";   // HTTP REST IP
int port = 8060;                  // HTTP REST TCP port 

# end of wifi config

unsigned long lastConnectionTime = 0;               // last time connected to the server, in milliseconds
const unsigned long postingInterval = 30L * 1000L;  // delay between updates, in milliseconds

WiFiEspClient client;           

const int RS485DE = 9;          //Low = Not transmitting
const int RS485REinv  = 8;      //Low = Receiving

unsigned long vectorLong[0x60]; // vector for multi byte parameters tmp storage

String logPost;                 // the logPost itself

boolean sendNow = false;        // true when logPost is complete and ready to send
byte sendToNibe = 0;             
byte n;                         // byte number indicator
boolean debug = false;
boolean printJSON = true;
//long year, month, day, hour, minute, seconds;
byte pLength, pValue1, pValue2, pValue3, pValue4;

void setup() {

  logPost.reserve(1280);

  Serial.begin(115200);
  Serial1.begin(19200);
  
  bitSet(UCSR1B, UCSZ12);     // Nibe 1130 bus do 9-bit com. Serial1 to 9-bit
  bitSet(UCSR1C, UCSZ11);     // Nibe 1130 bus do 9-bit com. Serial1 to 9-bit
  bitSet(UCSR1C, UCSZ10);     // Nibe 1130 bus do 9-bit com. Serial1 to 9-bit

  pinMode(RS485DE, OUTPUT);
  pinMode(RS485REinv, OUTPUT);
  digitalWrite(RS485DE, LOW);     //  RS485DE HIGH for transmit.
  digitalWrite(RS485REinv, LOW);  //  Always LOW. 
  
  pinMode(13, OUTPUT);
  
 
  // initialize  WifiESP shield
  
  Serial3.begin(9600);
  WiFi.init(&Serial3);
  
  // check for Wifi shield
  if (WiFi.status() == WL_NO_SHIELD) {      
    Serial.println("WiFi shield not present");
    while (true);
  }

  // connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  // connected so print out the data
  Serial.println("Connected to network");
  printWifiStatus();
 
}

void loop() {
  

  checkRS485Port();
  
 
  if ( sendNow && millis() - lastConnectionTime > postingInterval) {
    sendLogPost();
    sendNow = false;  
  }

/*
  delay(1000);
  sendLogPost();
*/
  digitalWrite(13, LOW);
}



void checkRS485Port() {
  int inByte;
  byte inCmd;
  static int state = 0;
  static byte totalMessageLength; // sent message length
  static byte messageLength;      // received message length
  static byte xxor;
  static byte paramNumber;
  static byte prevInByte;
  static byte q;

  if ( Serial1.available()) { 
   
    if ( UCSR1A & (1<<FE1) ) {
      //   Serial.print('!');
      digitalWrite(13, HIGH);
      return;
    }

    inCmd = bitRead(UCSR1B, 1);
    inByte  = Serial1.read();

    if ( inByte == -1 ) {
      Serial.print("<err>");
      return;
    }

    if ( debug ) {
      if ( state >= 30 ) {
        if ( inCmd == 1 ) {
          if ( n == 0 ) {
            Serial.println("#");
          }
          Serial.print('*');
          Serial.print(inByte, HEX);
          n = 1;
        }
        else {
          Serial.print(' ');
          Serial.print(inByte, HEX);
          n = 0;
        }
      }
    }

    if ( inCmd == 1  && state > 30) {
      Serial.print("[interrupted in state=");
      Serial.print(state);
      Serial.println("]");
      state = 0; 
    }

    if ( totalMessageLength > 0 && state > 60 ) { 
      if ( totalMessageLength == messageLength ) {
        state = 240; 
      }
    }

    messageLength++; 
    xxor ^= inByte;

    if ( 0 ) {
      Serial.print("[s=");
      Serial.print(state);
      Serial.print("]");
    }

    switch ( state ) {
    case 0:
      if ( inCmd == 1 ) {     
        if ( inByte == 0x00 ) {
          state += 10;
        }
      }
      break;

    case 10:
      if ( inCmd == 1 ) {       
        
        if ( inByte == 0x14 ) { // For RCU10 
          
          if ( sendToNibe ) {
            sendENQ();
            state = 300;
          }
          else {
            sendACK();
            state = 30;
          }
        }
        else 
          if ( inByte == 0x00 ) // Missed the start of a command
            ; 
          else
            state = 0;  
      } 
      else {
        state = 0;
      }
      break;

    case 20:
      if ( inByte == 0x06 ) { //0x06=ACK response read. 
        state += 10;
      }
      else {
        state = 250;
      }
      break;

    case 30:
      xxor = inByte; // cmd, always 0xC0 to RCU10
      if ( inByte == 0xC0 ) {
        if ( debug ) {
          Serial.println();
        }
        if ( debug ) printByte(inByte);
        state += 10;
      }
      else {
        Serial.print("[Got cmd 0x");
        Serial.print( inByte, HEX );
        Serial.print(", but expected 0xC0]");
        state = 0;
      }
      break;

    case 40:
      // always 0x00
      if ( inByte == 0 ) { 
        if ( debug ) printByte(inByte);
        state += 10;
        }
      else {
        Serial.print("[Got 0x");
        Serial.print( inByte, HEX );
        Serial.print(" in case 40, but expected 0x00]");
        state = 0;
      }
      break;

    case 50:
      // transmitter, always 0x24, Master?
      if ( inByte == 0x70 ) {
        if ( debug ) printByte(inByte);
        state += 10;
        }
      else {
        Serial.print("[Got transmitter 0x");
        Serial.print( inByte, HEX );
        Serial.print(" in case 50, but expected 0x24]");
        state = 0;
      }
      break;

    case 60: // length of message, after this byte, excl checksum
      totalMessageLength = inByte;
      messageLength = 0;
      if ( debug ) printByte(inByte);
      state += 10;
      break;

    case 70:
      // always 0x00
      if ( inByte == 0 ) {
        if ( debug ) printByte(inByte);
        state += 10;
        }
      else {
        if ( 0 ) {// catches the stray bytes that might come before checksum in the message
          Serial.print("[Got 0x");
          Serial.print( inByte, HEX );
          Serial.print(" in case 70, expected 0x00. Last paramNumber is 0x");
          Serial.print( paramNumber, HEX );
          Serial.print("]");
          //state = 0;
        }
      }
      break;

    case 80: // parameter number
      paramNumber = inByte;
      pLength = lookUpParameterLength( paramNumber );
      if ( debug ) printByte(inByte);
      state += 10;
      break;

    case 90: // parameter value high
      if ( pLength == 4 ) { // Four-byte value
        pValue4 = inByte; // Save high byte of value
        if ( debug ) printByte(inByte);
        state += 10;
      }
      
      if ( pLength == 2 ) { // Two-byte high value
        pValue2 = inByte; // Save high byte of value
        if ( debug ) printByte(inByte);
        state += 10;
      }
      
      if ( pLength == 1 ) {
        pValue1 = inByte;
        decodeParameter(paramNumber, 0, 0, 0, pValue1); // decode this message
        if ( debug ) printByte(inByte);
        state = 70; // begin receive next parameter
      }
      break;

    case 100: // parameter value high
      if ( pLength == 4 ) { // Four-byte value
        pValue3 = inByte; // Save high byte of value
        if ( debug ) printByte(inByte);
        state += 10;
      }
      
      if ( pLength == 2 ) { // Two-byte low value
        pValue1 = inByte;
        decodeParameter(paramNumber, 0, 0, pValue2, pValue1); // decode this message
        if ( debug ) printByte(inByte);
        state = 70; // begin receive next parameter
      }
      break;

    case 110: // parameter value high
      if ( pLength == 4 ) { // Four-byte value
        pValue2 = inByte;  
        if ( debug ) printByte(inByte);
        state += 10;
      }
      break;

    case 120: // parameter value low
      if ( pLength == 4 ) { // Four-byte value
        pValue1 = inByte; 
        decodeParameter(paramNumber, pValue4, pValue3, pValue2, pValue1); // decode this message
        if ( debug ) printByte(inByte);
        state = 70; // begin receive next parameter
      }
      break;

    case 240:
      totalMessageLength = 0;
      if ( 1 ) {
        if (xxor != 0 )
          Serial.print ("[XOR=NOK]");
        //       else
        //         Serial.print ("[XOR=OK]");
      }
      sendACK(); // Thanks for letting me know!
      state = 245;
      break;

    case 245: // Receive 0x03 ETX from Master
      state = 0;
      break;

    case 300: 
      // sendToNibe - Waiting for 0x06 from master CPU.
      if ( inByte == 0x06 ) {
       // send......
        state += 10;
      }
      else {
        if ( 1 ) {
          Serial.print("[Got 0x");
          Serial.print( inByte, HEX );
          Serial.print(" in case 300, expected 0x06. Clears sendToNibe.");
          Serial.print("]");
          state = 0;
          sendToNibe = 0;
        }
      }
      break;

    }
  
  }
}

byte lookUpParameterLength ( byte paramNumber ) {
  switch ( paramNumber ) {
  case 0x08:
  case 0x1f:
  case 0x21:
  case 0x2e:
    return 4;
    break;

  case 0x01:
  case 0x09:
  case 0x0a:
  case 0x10:
  case 0x12:
  case 0x13:
  case 0x14:
  case 0x15:
  case 0x1a:
  case 0x1b:
  case 0x1c:
  case 0x1d:
  case 0x1e:
  case 0x22:
  case 0x23:
  case 0x24:
  case 0x25:
  case 0x26:
  case 0x27:
  case 0x28:
  case 0x2a:
  case 0x2b:
  case 0x2c:
  case 0x37:
  case 0x39:
  case 0x3c:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47:
  case 0x48:
  case 0x49:
  case 0x51:
  case 0x52:
  case 0x53:
  case 0x54:
  case 0x5d:
  case 0x5e:
  case 0x5f:
    return 2; 
    break;
 
  case 0x00:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:
  case 0x0b:
  case 0x0c:
  case 0x0d:
  case 0x0e:
  case 0x0f:
  case 0x11:
  case 0x16:
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x20:
  case 0x29:
  case 0x2d:
  case 0x2f:
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x38:
  case 0x3a:
  case 0x3b:
  case 0x3d:
  case 0x3e:
  case 0x3f:
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x4a:
  case 0x4b:
  case 0x4c:
  case 0x4d:
  case 0x4e:
  case 0x4f:
  case 0x50:
  case 0x55:
  case 0x56:
  case 0x57:
  case 0x58:
  case 0x59:
  case 0x5a:
  case 0x5b:
  case 0x5c:
  case 0x60:
  return 1;
    break;
  
  default:
    Serial.print("[unknown data length for=");
    Serial.print(paramNumber,HEX);
    Serial.println("]");
    return 2;
  }
}


void decodeParameter(byte paramNumber, byte pV4, byte pV3, byte pV2, byte pV1) {

  int pValue;  
  String sValue;
  
  
  if ( paramToFollow(paramNumber) && !debug ) {

    switch ( paramNumber ) {
      case 0x01:  // Hot Water Temp

          sendNow = false;
          logPost = "{ ";

          pValue = pV2*0x100+pV1;
          addJSON("HWTemp",(String)((float)pValue/10.0));        
        break;

      case 0x02: // Start temp.HW
          addJSON("HWStartTemp",(String) pV1);
        break;
      
      case 0x03: // Stop temp.HW
          addJSON("HWStopTemp",(String) pV1);
        break;

      case 0x04: // Stop temp XHW
          addJSON("XHWStopTemp",(String) pV1);
        break;
      
      case 0x05: // Stop compressor XHW
          addJSON("XHWStopCompressor",(String) pV1);
        break;
      
      case 0x06: // Interval XHW
          addJSON("HWXIntervall",(String) pV1);
        break;
            
      case 0x07: // HW Running Time m 
           vectorLong[paramNumber] = pV1;
        break;

      case 0x08: // HW Running Time h
          vectorLong[paramNumber] = 0;
          vectorLong[paramNumber] += long(pV4) << 24;
          vectorLong[paramNumber] += long(pV3) << 16;
          vectorLong[paramNumber] += long(pV2) << 8;
          vectorLong[paramNumber] += long(pV1);

          sValue = vectorLong[paramNumber];
          sValue = sValue + '.' + vectorLong[0x07]; 
          addJSON("HWRunTime", sValue);
        break;
      
      case 0x09: // Flow Temperature
          pValue = pV2*0x100+pV1;
          addJSON("FlowTempOut",(String)((float)pValue/10.0));
        break;

      case 0x0A: // Calculated Flow Temperature
          pValue = pV2*0x100+pV1;
          addJSON("FlowTempCalc",(String)((float)pValue/10.0));        
        break;

      case 0x0B: //Curve coefficient
          addJSON("Curve",(String) pV1);
        break;
      
      case 0x0C: // Offset heating curve
          addJSON("CurveOffset",(String) pV1);
        break;
      
      case 0x0D: // Flow temp. MIN
          addJSON("FlowTempMin",(String) pV1);
        break;
      
      case 0x0E: // Flow temp. MIN
          addJSON("FlowTempMax",(String) pV1);
        break;
      
      case 0x0F: // External adjustment
          addJSON("ExtAdjustment",(String) pV1);
        break;
      
      case 0x10: // Return Flow Temperature
          pValue = pV2*0x100+pV1;
          addJSON("FlowTempReturn",(String)((float)pValue/10.0));
        break;

      case 0x11: // Return Flow Temp. Max
          addJSON("FlowTempReturnMax",(String) pV1);
        break;
      
      case 0x12: // Degree minutes
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;
          addJSON("DegreeMinutes", (String) pValue);
        break;

      case 0x13:
          pValue = pV2*0x100+pV1;
          addJSON("0x13",(String) pValue);
        break;

      case 0x14:
          pValue = pV2*0x100+pV1;
          addJSON("0x14",(String)((float)pValue/10.0));
        break;

      case 0x15:
          addJSON("0x15",(String) pV1);
        break;

      case 0x16:
          addJSON("0x16",(String) pV1);
        break;

      case 0x17:
          addJSON("0x17",(String) pV1);
        break;

      case 0x18:
          addJSON("0x18",(String) pV1);
        break;

      case 0x19:
          addJSON("0x19",(String) pV1);
        break;

      case 0x1A:
          pValue = pV2*0x100+pV1;
          addJSON("0x1A",(String) pValue);
        break;
    
      case 0x1B: // Outdoor Temperature
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;          
          addJSON("OutdoorTemp",(String)((float)pValue/10.0));
        break;

      case 0x1C: // Average Outdoor Temp 24h
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;
          addJSON("OutdoorTempAvg24h",(String)((float)pValue/10.0));
          
        break;

      case 0x1D: // Brine Temp In
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;        
          addJSON("BrineTempIn",(String)((float)pValue/10.0));
        break;
  
      case 0x1E: // Brine Temp Out 
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;
          addJSON("BrineTempOut",(String)((float)pValue/10.0));
        break;
      
      case 0x1F: // Number of starts for compressor
          vectorLong[paramNumber] = 0;
          vectorLong[paramNumber] += long(pV4) << 24;
          vectorLong[paramNumber] += long(pV3) << 16;
          vectorLong[paramNumber] += long(pV2) << 8;
          vectorLong[paramNumber] += long(pV1);
          addJSON("CompressorStarts",(String)vectorLong[paramNumber]);
        break;
      
      case 0x20: // Comp. acc. run time minutes
          vectorLong[paramNumber] = pV1;
        break;

      case 0x21: // Comp. acc. run time hours
          vectorLong[paramNumber] = 0;
          vectorLong[paramNumber] += long(pV4) << 24;
          vectorLong[paramNumber] += long(pV3) << 16;
          vectorLong[paramNumber] += long(pV2) << 8;
          vectorLong[paramNumber] += long(pV1);

          sValue = vectorLong[paramNumber];
          sValue = sValue + '.' + vectorLong[0x20]; 
          addJSON("CompRunTime", sValue);
        break;
      
      case 0x22: // Hot gas temp
          pValue = pV2*0x100+pV1;
          addJSON("HGTemp",(String)((float)pValue/10.0));
        break;
      
      case 0x23: // Liquid line temp.
          pValue = pV2*0x100+pV1;          
          addJSON("LiquidLineTemp",(String)((float)pValue/10.0));
        break;

      case 0x24: // Bulb temperature
          pValue = pV2*0x100+pV1;
          addJSON("BulbTemp",(String)((float)pValue/10.0));
        break;

      case 0x25: // Cond out temp
          pValue = pV2*0x100+pV1;
          addJSON("CondTemp",(String)((float)pValue/10.0));
        break;

      case 0x26:
          pValue = pV2*0x100+pV1;
          addJSON("0x26",(String) pValue);
        break;

      case 0x27:
          pValue = pV2*0x100+pV1;
          addJSON("0x27",(String) pValue);
        break;

      case 0x28:
          pValue = pV2*0x100+pV1;
          addJSON("0x28",(String) pValue);
        break;

      case 0x29:
          addJSON("0x29",(String) pV1);
        break;

      case 0x2A: // Current Phase 1
          pValue = pV2*0x100+pV1;
          addJSON("Phase1Amp",(String)((float)pValue/10.0));
        break;
      
      case 0x2B: // Current Phase 2
          pValue = pV2*0x100+pV1;
          addJSON("Phase2Amp",(String)((float)pValue/10.0));
        break;

      case 0x2C: // Current Phase 3
          pValue = pV2*0x100+pV1;
          addJSON("Phase3Amp",(String)((float)pValue/10.0));
        break;

      case 0x2D: // Electric Add heat run time min
          vectorLong[paramNumber] = pV1;
        break;
      
      case 0x2E: // Electric Add heat run time h */
          vectorLong[paramNumber] = 0;
          vectorLong[paramNumber] += long(pV4) << 24;
          vectorLong[paramNumber] += long(pV3) << 16;
          vectorLong[paramNumber] += long(pV2) << 8;
          vectorLong[paramNumber] += long(pV1);

          sValue = vectorLong[paramNumber];
          sValue = sValue + '.' + vectorLong[0x2D]; 
          addJSON("AddHeatingRunTime", sValue);
        break;

      case 0x2F:
          addJSON("0x2F",(String) pV1);
        break;

      case 0x30:
          addJSON("0x30",(String) pV1);
        break;
               
      case 0x31:
          addJSON("0x31",(String) pV1);
        break;
               
      case 0x32:
          addJSON("0x32",(String) pV1);
        break;
               
      case 0x33:
          addJSON("0x33",(String) pV1);
        break;
               
      case 0x34:
          addJSON("0x34",(String) pV1);
        break;
               
      case 0x35:
          addJSON("0x35",(String) pV1);
        break;
               
      case 0x36:
          addJSON("0x36",(String) pV1);
        break;
               
      case 0x37:
          pValue = pV2*0x100+pV1;
          addJSON("0x37",(String) pValue);
        break;
               
      case 0x38:
          addJSON("0x38",(String) pV1);
        break;
               
      case 0x39:
          pValue = pV2*0x100+pV1;
          addJSON("0x39",(String) pValue);
        break;
               
      case 0x3A:
          addJSON("0x3A",(String) pV1);
        break;
               
      case 0x3B:
          addJSON("0x3B",(String) pV1);
        break;
               
      case 0x3C:
          addJSON("0x3C",(String) pV1);
        break;
               
      case 0x3D:
          addJSON("0x3D",(String) pV1);
        break;
               
      case 0x3E:
          addJSON("0x3E",(String) pV1);
        break;
               
      case 0x3F:
          addJSON("0x3F",(String) pV1);
        break;
               
      case 0x40:
          addJSON("0x40",(String) pV1);
        break;
               
      case 0x41:
          addJSON("0x41",(String) pV1);
        break;
               
      case 0x42:
          addJSON("0x42",(String) pV1);
        break;
               
      case 0x43:
          pValue = pV2*0x100+pV1;
          addJSON("0x43",(String) pValue);
        break;
               
      case 0x44:
          pValue = pV2*0x100+pV1;
          addJSON("0x44",(String) pValue);
        break;
               
      /*  // using logstash timesamp instead
      case 0x45: // year
          year = pV2*0x100+pV1;
        break; 
      
      case 0x46: // month
          month =  pV2*0x100+pV1;
        break;  
      
      case 0x47: // day
          day = pV2*0x100+pV1;
        break; 
      
      case 0x48: // hour
          hour  = pV2*0x100+pV1;
        break;
      
      case 0x49: // minutes
          minute  = pV2*0x100+pV1;
        break; 
      
      case 0x4A: // seconds ??
          seconds = pV2*0x100+pV1;
        break;
      */

      case 0x4B: // ??
          addJSON("0x4B",(String) pV1);
        break;
               
      case 0x4C: // Display language
          addJSON("DispLang",(String) pV1);
        break;
               
      case 0x4D: // Display contrast
          addJSON("DispContrast",(String) pV1);
        break;
               
      case 0x4E: // Display intensity
          addJSON("DispIntensity",(String) pV1);
        break;
               
      case 0x4F:
          addJSON("0x4F",(String) pV1);
        break;
               
      case 0x50: // Summer mode temp
          addJSON("SummerModeTemp",(String) pV1);
        break;
               
      case 0x51: // Diff Add. heating 
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;          
          addJSON("AddHeatingDiff",(String) pValue);
        break;
      
      case 0x52: // Start Add. heating
          pValue = pV2*0x100+pV1;
          if (pValue > 0x8000) pValue = 0xFFFF - pValue;
          addJSON("AddHeatingStart",(String) pValue);
        break;
      
      case 0x53:
          addJSON("0x53",(String) pV1);
        break;

      case 0x54:
          addJSON("0x54",(String) pV1);
        break;

      case 0x55:
          addJSON("0x55",(String) pV1);
        break;

      case 0x56: // Flow diff HP
          addJSON("FlowdiffHP",(String) pV1);
        break;

      case 0x57: // Diff HP Add
          addJSON("DiffHPAdd",(String) pV1);
        break;

      case 0x58: // Floor drying
          addJSON("FloorDrying",(String) pV1);
        break;

      case 0x59: // Numb. of days per. 1
          addJSON("FloorDryingNumDaysPer1",(String) pV1);
        break;

      case 0x5A: // Temp. period 1
          addJSON("FloorDryingTempPer1",(String) pV1);
        break;

      case 0x5B: // Numb. of days per. 2
          addJSON("FloorDryingNumDaysPer2",(String) pV1);
        break;

      case 0x5C: // TEMP PERIOD 2
          addJSON("FloorDryingTempPer1",(String) pV1);
        break;

      case 0x5D:
          addJSON("0x5D",(String) pV1);
        break;

      case 0x5E:
          addJSON("0x5E",(String) pV1);
        break;

      case 0x5F:
          addJSON("0x5F",(String) pV1);
        break;

      case 0x60: // Always 0 ?? I follow it for last key value detection only
          //addJSON("0x60",(String) pV1);
          sendNow = true;
          logPost = logPost + "\"0x60\":" + (String) pV1 + " }";
        break;
    }
  }
}

void sendACK() {

  digitalWrite(RS485REinv, HIGH); // stop receive
  digitalWrite(RS485DE, HIGH);    // start transmit
  bitSet(UCSR1B, 0);              // set 8-bit

  Serial1.write(0x06);
  Serial1.flush();

  digitalWrite(RS485DE, LOW);
  digitalWrite(RS485REinv, LOW);
}

void sendENQ() {

  digitalWrite(RS485REinv, HIGH); // stop receive
  digitalWrite(RS485DE, HIGH);    // start transmit
  bitSet(UCSR1B, 0);              // set 8-bit

  Serial1.write(0x05);
  Serial1.flush();

  digitalWrite(RS485DE, LOW);
  digitalWrite(RS485REinv, LOW);
}

boolean paramToFollow ( byte paramNumber ) {

  switch ( paramNumber ) {
    case 0x01: // HWTemp
    case 0x02: // HWStartTemp
    case 0x03: // HWStopTemp
    case 0x04: // XHWStopTemp
    case 0x05: // XHWStopCompressor
    case 0x06: // HWXIntervall
    case 0x07: // HW Running Time m
    case 0x08: // HW Running Time h
    case 0x09: // Flow Temperature
    case 0x0A: // Flow Temperature Claculated
    case 0x0B: // Curve
    case 0x0C: // CurveOffset
    case 0x0D: // FlowTempMin
    case 0x0E: // FlowTempMax
    case 0x0F: // ExtAdjustment
    case 0x10: // FlowTempReturn
    case 0x11: // FlowTempReturnMax
    case 0x12: // Degree minutes
    case 0x13: // ??  -- -32768
    //case 0x14: // Flow temp. system 2
    //case 0x15: // Curve 2
    //case 0x16: // CurveOffset 2
    //case 0x17: // Flow Temp Min 2
    //case 0x18: // Flow Temp Max 2
    case 0x19: // ??   -- 0
    case 0x1A: // ??   -- -32768
    case 0x1B: // Outdoor Temp
    case 0x1C: // Outdor Temp (avg)
    case 0x1D: // Brine in
    case 0x1E: // Brine out
    case 0x1F: // Number of Compressor Starts
    case 0x20: // Comp. acc. run time m 
    case 0x21: // Comp. acc. run time h
    case 0x22: // Hot Gas Temp
    case 0x23: // Liquid Line Temp
    case 0x24: // Bulb Temp
    case 0x25: // Cond out temp
    case 0x26: // ??  -- 0
    case 0x27: // ??  -- 0
    case 0x28: // ??  -- 6400
    case 0x29: // ??  -- 0
    case 0x2A: // Current Phase 1
    case 0x2B: // Current Phase 1
    case 0x2C: // Current Phase 1
    case 0x2D: // Add Heating Run Time m
    case 0x2E: // Add Heating Run Time h
    case 0x2F: // ??  -- 0
    case 0x30: // ??  -- 0
    case 0x31: // ??  -- 0
    case 0x32: // ??  -- 0
    case 0x33: // ??  -- 0
    case 0x34: // ??  -- 0
    case 0x35: // ??  -- 3
    case 0x36: // ??  -- 0
    case 0x37: // ??  -- -32768
    case 0x38: // ??  -- 10
    case 0x39: // ??  -- 0 - 24592 
    case 0x3A: // ??  -- 64 - 108
    case 0x3B: // ??  -- 43
    case 0x3C: // ??  -- 0
    case 0x3D: // ??  -- 0
    case 0x3E: // ??  -- 0
    case 0x3F: // ??  -- 0
    case 0x40: // ??  -- 1
    case 0x41: // ??  -- 1
    case 0x42: // ??  -- 0
    case 0x43: // ??  -- -32768
    case 0x44: // ??  -- -32768  
    
    //case 0x45: // Year
    //case 0x46: // Month
    //case 0x47: // Day
    //case 0x48: // Hour
    //case 0x49: // Minute
    //case 0x4A: // Seconds 
    
    case 0x4B: // ??  -- 0
    //case 0x4C: // Display Language
    //case 0x4D: // Display Contrast
    //case 0x4E: // Display Initensity
    case 0x4F: // ??  -- 0
    case 0x50: // Summer Mode Temp
    case 0x51: // Add Heating (diff degree minutes)
    case 0x52: // Add Heating (start degree minutes)
    case 0x53: // ??  -- 25
    case 0x54: // ??  -- 3
    case 0x55: // ??  -- 156
    case 0x56: // Flow diff HP
    case 0x57: // Diff HP Add
    // case 0x58: // Floor drying set
    // case 0x59: // Numb. of days per. 1
    // case 0x5A: // Temp. period 1
    // case 0x5B: // Numb. of days per. 2
    // case 0x5C: // Temp. period 2
    case 0x5D: // ??  -- 0
    case 0x5E: // ??  -- 0
    case 0x5F: // ??  -- 0
    case 0x60: // ??  -- 0
      return true;
      break;
  }
  return false;
}

void printByte(int input) {

  if ( input < 16 )
      Serial.print('0');
  Serial.print(input, HEX);
  Serial.print(' ');

}

void sendLogPost() {

  client.stop();

  int logPostLength = logPost.length();
  
  Serial.print("logPostLength="); Serial.println(logPostLength); 
  
  if (client.connect(server, 8060)) {
    Serial.println("Connected to server");
    
    client.println("POST /logstash HTTP/1.1");
    client.println("Host: 192.168.1.90:8060");
    client.println("Connection: close");

    client.println("Content-Type: application/json");
    client.print("Content-Length:"); client.println(logPostLength);
    client.println();
    client.println(logPost);
  
    lastConnectionTime = millis();
    Serial.println("sendLogPost done");

  } else {
    Serial.println("sendLogPost failed");
  }
  
  if (printJSON) {
    Serial.println(logPost);
  }
}

void printWifiStatus() {

  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print(" IP Address: ");
  Serial.print(ip);

  long rssi = WiFi.RSSI();
  Serial.print(" RSSI:");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void addJSON(String description, String pValue) {

  logPost = logPost + "\"";
  logPost = logPost + description + "\":";
  logPost = logPost + pValue + ",";
}

void addJSONString(String description, String pValue) {

  logPost = logPost + "\"";
  logPost = logPost + description + "\":";
  logPost = logPost + "\"";
  logPost = logPost + pValue + "\",";
}







