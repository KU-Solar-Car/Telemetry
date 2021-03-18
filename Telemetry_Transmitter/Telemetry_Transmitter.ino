#include "DueCANLayer.h"
#include "XBee.h"
#include "MonitoredSerial.h"
#include "Stats.h"
#include "IPAddress.h"
#include "Frames.h"

const byte BYTE_MIN = -128;
byte maxTemp;

const unsigned long DELAY = 5000;
unsigned long nextTimeWeSendFrame;
MonitoredSerial mySerial(Serial1, Serial);
XBee xbee(mySerial);

const size_t REQUEST_BUFFER_SIZE = 488;
char requestBuffer[REQUEST_BUFFER_SIZE];

Stats testStats;

void setup()
{
  // Set the serial interface baud rate
  Serial.begin(9600);
  Serial1.begin(9600);

  /* --------------------------------
   * Set the XBee in API Mode
   * =================================*/
  if(xbee.configure())
    Serial.println("Configuration successful");
  else
    Serial.println("Configuration failed");
    
  /* =================================
   * Initialize CAN board
   * =================================*/
  if(canInit(0, CAN_BPS_250K) == CAN_OK)
    Serial.print("CAN0: Initialized Successfully.\n\r");
  else
    Serial.print("CAN0: Initialization Failed.\n\r");
  
  /* =================================
   * Initialize StatData
   * =================================*/
  testStats[StatKey::BATT_VOLTAGE].present = true;
  testStats[StatKey::BATT_VOLTAGE].doubleVal = 420.69;
  for (int k = 1; k < StatKey::_LAST; k++)
  {
    //test_stats[i] = {false, {i, .boolVal=false}};
    testStats[k].present = false;
  }
   
  /* =================================
   * Wait for modem to associate before starting 
   * =================================*/
  userFrame status;
//  Serial.println("Waiting for network to associate...");
//  do
//  {
//    status = xbee.read();
//  } while(!(status.frameType == 0x8A && status.frameData[0] == 2));
//  Serial.println("Network associated.");
  
  /* =================================
   * Initialize variables that track stuff
   * =================================*/
  maxTemp = -128;
  nextTimeWeSendFrame = 0;
}

void loop()
{
  // sendMaxTempEveryFiveSeconds();
  
  printReceivedFrame();
  sendStatsEveryFiveSeconds(testStats);
  shutdownOnCommand();
}



void printReceivedFrame()
{
  userFrame recvd = xbee.read();
  if (!(recvd == NULL_USER_FRAME))
  {
    Serial.println("Got frame:");
    Serial.println("Frame type: " + String(recvd.frameType, HEX));
    Serial.print("Frame data: ");
    Serial.write(recvd.frameData, recvd.frameDataLength);
    Serial.println("");
  }
  // else
    // Serial.println("Got here nothing :(");
}

void shutdownOnCommand()
{
  if (Serial.read() == 's')
  {
    Serial.println("Shutting down, please wait up to 2 minutes...");
    if (Serial.read() != 'c')
      xbee.shutdown(120000);
    else
    {
      if (xbee.shutdownCommandMode())
        Serial.println("Shutdown successful");
      else
        Serial.println("Shutdown failed");
    }
  }
}


void setContentLengthHeader(char* dest, int len)
{
  char* contentLength = strstr(dest, "Content-Length: ") + 16;
  char tmpBuffer[4]; 
  sprintf(tmpBuffer, "%03u", len);
  strncpy(contentLength, tmpBuffer, 3);
}

void sendStatsEveryFiveSeconds(Stats stats)
{
  unsigned long myTime = millis();
  if (myTime >= nextTimeWeSendFrame)
  {
    // mySerial.suppress();
    nextTimeWeSendFrame = myTime + DELAY;

    if (xbee.isConnected())
      sendStats(stats);
    else
      Serial.println("Modem is not connected, skipping this time.");
    Serial.println("");
    mySerial.unsuppress();
  }
}

void sendStats(Stats stats)
{
  
  strcpy(requestBuffer, "POST /car HTTP/1.1\r\nContent-Length: 000\r\nHost: ku-solar-car-b87af.appspot.com\r\nContent-Type: application/json\r\nAuthentication: eiw932FekWERiajEFIAjej94302Fajde\r\n\r\n");
  strcat(requestBuffer, "{");
  int bodyLength = 2;
  for (int k = 0; k < StatKey::_LAST; k++)
  {
    if (stats[k].present)
    {
      bodyLength += toKeyValuePair(requestBuffer + strlen(requestBuffer), k, stats[k]); // append the key-value pair
      strcat(requestBuffer, ",");
    }
  }
  
  strcat(requestBuffer, "}");
  setContentLengthHeader(requestBuffer, bodyLength);

  Serial.println(requestBuffer);
  xbee.sendTCP(IPAddress(216, 58, 192, 212), 443, 0, 0, requestBuffer, strlen(requestBuffer));

  // strcpy(requestBuffer, "GET /get HTTP/1.1\r\nHost: httpbin.org\r\n");
  // xbee.sendTCP(IPAddress(54, 166, 163, 67), 443, 0, 0, requestBuffer, strlen(requestBuffer));
}

int toKeyValuePair(char* dest, int key, StatData data)
{
  switch(key)
  {
    case StatKey::BATT_VOLTAGE: return sprintf(dest, "\"battery_voltage\":%6f", data.doubleVal); break;
    case StatKey::BATT_CURRENT: return sprintf(dest, "\"battery_current\":%6f", data.doubleVal); break;
    case StatKey::BATT_TEMP: return sprintf(dest, "\"battery_temperature\":%6f", data.doubleVal); break;
    case StatKey::BMS_FAULT: return sprintf(dest, "\"bms_fault\":%d", data.boolVal); break;
    case StatKey::GPS_TIME: return sprintf(dest, "\"gps_time\":%6f", data.uIntVal); break;
    case StatKey::GPS_LAT: return sprintf(dest, "\"gps_lat\":%6f", data.doubleVal); break;
    case StatKey::GPS_LON: return sprintf(dest, "\"gps_lon\":%6f", data.doubleVal); break;
    case StatKey::GPS_VEL_EAST: return sprintf(dest, "\"gps_velocity_east\":%6f", data.doubleVal); break;
    case StatKey::GPS_VEL_NOR: return sprintf(dest, "\"gps_velocity_north\":%6f", data.doubleVal); break;
    case StatKey::GPS_VEL_UP: return sprintf(dest, "\"gps_velocity_up\":%6f", data.doubleVal); break;
    case StatKey::GPS_SPD: return sprintf(dest, "\"gps_speed\":%6f", data.doubleVal); break;
    case StatKey::SOLAR_VOLTAGE: return sprintf(dest, "\"solar_voltage\":%6f", data.doubleVal); break;
    case StatKey::SOLAR_CURRENT: return sprintf(dest, "\"solar_current\":%6f", data.doubleVal); break;
    case StatKey::MOTOR_SPD: return sprintf(dest, "\"motor_speed\":%6f", data.doubleVal); break;
  }
}


/* =================================
 * Temporarily not being used
 * =================================*/
//void sendMaxTempEveryFiveSeconds()
//{
//  // Check for received message
//  long lMsgID;
//  bool bExtendedFormat;
//  byte cRxData[8];
//  byte cDataLen;
//  if(canRx(0, &lMsgID, &bExtendedFormat, &cRxData[0], &cDataLen) == CAN_OK)
//  {
//    if (lMsgID == 0x6B1) {
//      if (cRxData[4] > maxTemp)
//        maxTemp = cRxData[4];
//    }
//  } // end if
//  if (millis() >= nextTimeWeSendFrame)
//  {
//    nextTimeWeSendFrame += DELAY;
//    printTemperature(maxTemp);
//    maxTemp = BYTE_MIN;
//  }
//}

//void printTemperature(byte temp)
//{
//  Serial.print("High Temperature: ");
//  Serial.print(temp);
//  Serial.print("\n\r");
//}
