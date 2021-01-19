/* File: XBee.h
 * Author: Andrew Riachi
 * Date: 2020-10-15
 * Description: Functions to help send packets using the Digi module
 */
#ifndef XBEE_H
#define XBEE_H

#include <Arduino.h>
#include "MonitoredSerial.h"
#include "Frames.h"



class XBee
{
  private:
  MonitoredSerial& m_serial;
  frame m_rxBuffer;

  public:
  XBee(MonitoredSerial& serial) : m_serial(serial){}
  bool configure();
  userFrame read(); // Can read one full packet at a time.
  bool shutdownCommandMode();
  void shutdown(unsigned int timeout);
  void sendFrame(const byte& frameType, const char frameData[], size_t frameDataLen);
  void sendATCommand(uint8_t frameID, const char command[], const char param[], size_t paramLen);
};

#endif
