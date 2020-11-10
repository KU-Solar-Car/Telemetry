/* File: XBee.cpp
 * Author: Andrew Riachi
 * Date: 2020-10-15
 * Description: Implementation of the XBee class
 */
#include "XBee.h"
#include <Arduino.h>

packet::packet()
{
  // Helps prevent heap fragmentation (even though it's very possible in the rest of my code :/)
  frameData.reserve(1550);
}

bool XBee::configure(const String& server)
{
  m_serial.write("+++");
  delay(1500);
  String ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("ATAP 1\r");
  delay(2000);
  ok = "";
  ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("CN\r");
  return true;
}
void XBee::sendFrame(byte frameType, const String& frameData)
{
  uint16_t checksum = frameType;
  uint16_t len = frameData.length() + 1;
  for (uint16_t i = 0; i < len; i++)
    checksum += frameData[i];
  checksum = 0xFF - checksum;

  const byte start = 0x7E;
  byte len_msb = (byte) (len >> 8);
  byte len_lsb = (byte) len;

  SerialUSB.println("Length of frameData: " + String(len));
  SerialUSB.println("MSB: " + String(len_msb));
  SerialUSB.println("LSB: " + String(len_lsb));

  m_serial.write(start);
  m_serial.write(len_msb);
  m_serial.write(len_lsb);
  m_serial.write(frameData.c_str());
  m_serial.write(checksum);


  // debug
  /*
  m_serial.print(start);
  m_serial.print((int) (len_msb << 8) + len_lsb);
  m_serial.print(frameType);
  m_serial.print(frameData.c_str());
  m_serial.print(checksum);
  */
}

void XBee::sendATCommand(const String& command, const String& param)
{
  SerialUSB.println("Length of command: " + String(command.length()));
  SerialUSB.println("Length of param: " + String(param.length()));
  SerialUSB.println("Length of frame type: " + String(String((char)0x08).length()));
  sendFrame(0x08, command+param);
}

void XBee::shutdown()
{
  sendATCommand("SD", "0");
}

bool XBee::shutdownCommandMode()
{
  m_serial.write("+++");
  delay(1500);
  String ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("ATSD 0\r");
  m_serial.setTimeout(30000);
  ok = "";
  ok = m_serial.readStringUntil('\r');
  m_serial.setTimeout(1000);
  if (ok != "OK")
    return false;
  m_serial.write("CN\r");
  return true;
}

userPacket XBee::read()
{
  // To save memory, I would like userPacket to contain references to m_rxBuffer.
  // Therefore, once we get a non-empty userPacket, we need it to be valid at least until the next call
  // to read().
  // So here, we'll check if our last call to read() received a full packet and if so, clear m_rxBuffer
  // to make room for a brand new packet. (This means userPacket will no longer have your previous packet!)
  if (m_rxBuffer.recvd_checksum)
  {
    m_rxBuffer = {};
  }
  // find the first field that hasn't been received yet.
  // if we don't have the full packet yet, return ""
  for (int recvd = m_serial.read(); (recvd != -1 && !m_rxBuffer.recvd_checksum); recvd = m_serial.read())
  {
    if (!m_rxBuffer.recvd_start)
    {
      m_rxBuffer.recvd_start = (recvd == 0x7E);
    }
    else if (!m_rxBuffer.recvd_len_msb)
    {
      m_rxBuffer.recvd_len_msb = true;
      m_rxBuffer.length = recvd << 8;
    }
    else if (!m_rxBuffer.recvd_len_lsb)
    {
      m_rxBuffer.recvd_len_lsb = true;
      m_rxBuffer.length += recvd;
    }
    else if (!m_rxBuffer.recvd_frameType)
    {
      m_rxBuffer.recvd_frameType = true;
      m_rxBuffer.frameType = recvd;
    }
    else if (m_rxBuffer.frameData.length() != m_rxBuffer.length-1) // -1 to account for the frameType
    {
      m_rxBuffer.frameData += (char) recvd; // use a char here because that is probably what append is
      // defined for
    }
    else
    {
      m_rxBuffer.recvd_checksum = true;
      m_rxBuffer.checksum = recvd;
    }
  }
  // if we are done looping because we received the checksum
  if (m_rxBuffer.recvd_checksum)
  {
    // calculate the checksum
    uint8_t my_checksum = m_rxBuffer.frameType; // frameType is technically the start of frameData
    for (unsigned int i = 0; i < m_rxBuffer.frameData.length(); i++)
    {
      my_checksum += (uint8_t) m_rxBuffer.frameData.charAt(i);
    }
    if (m_rxBuffer.recvd_checksum == my_checksum)
      return userPacket{m_rxBuffer.frameType, m_rxBuffer.frameData};
    else // poo poo packet, clear the buffer and return nothing to our poor user :(
      m_rxBuffer = {};
  }
  return NULL_USER_PACKET;
}
