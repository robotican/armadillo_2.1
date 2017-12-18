#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <Arduino.h>
#include "protocol.h"

class Communicator
{

  public:
  
  enum State 
  {
    HEADER_PART_A,
    HEADER_PART_B,
    PACKAGE
  };
  
  void init(int baudrate)
  {
    Serial.begin(baudrate);
  }

  /* return -1 for reading in process, or type of the incoming pkg */
  int read(byte buff[]) 
  {
    switch (state_)
    {
      case HEADER_PART_A: //read header
      {
        if (tryReadHeader())
          state_ = HEADER_PART_B;
        break;
      }
      case HEADER_PART_B: //read pkg size
      {
        Serial.println("B");
        pkg_size_ = tryReadPkgSize();
        if (pkg_size_ != -1)
          state_ = PACKAGE;
        break;
      }
      case PACKAGE:
      {
        int incoming = Serial.read();
        if (incoming != -1)
          buff[pkg_indx_++] = (byte)incoming;

        if (pkg_indx_ >= pkg_size_) //done reading
        {
          protocol::package pkg;
          fromBytes(buff, sizeof(protocol::package), pkg);
          return (int)pkg.type;
          reset();
        }
        break;
      }
    }
    return -1;
  }

  static void fromBytes(byte buff[], size_t pkg_size, protocol::package &pkg)
  {
    memcpy(&pkg, buff, pkg_size);
  }


  private:
  State state_ = HEADER_PART_A;
  int pkg_indx_ = 0;
  int pkg_size_ = 0;
  
  void reset()
  {
    state_ = HEADER_PART_A;
    pkg_indx_ = 0;
    pkg_size_ = 0;
  }
  
  /* try to read valid header start */
  bool tryReadHeader()
  {
    if (Serial.available()>0 && Serial.read() == protocol::HEADER_CODE)
       return true;
    return false;
  }

  /* return -1 for failure and pkg size as success */
  int tryReadPkgSize()
  {
    if (Serial.available()>0)
      return Serial.read();
    return -1;
  }
};
    
#endif //COMMUNICATOR_H
