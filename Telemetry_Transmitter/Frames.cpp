#include "Frames.h"

frame& frame::operator=(const frame& other)
{
  bytes_recvd = other.bytes_recvd;
  strncpy(buf, other.buf, other.length());
  memset(buf+other.length(),0,MAX_BUFFER_SIZE);
}

void frame::clear()
{
  memset(buf,0,MAX_BUFFER_SIZE);
}

frame::~frame()
{
  clear();
}

bool userFrame::operator==(const userFrame& other)
{
  return (
    frameType == other.frameType
    && frameDataLength == other.frameDataLength
    && frameData == other.frameData
  );
}
