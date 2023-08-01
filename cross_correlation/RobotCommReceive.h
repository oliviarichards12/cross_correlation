#ifndef CU_ROBOTICS_ROBOT_COMM_RECEIVE_H
#define CU_ROBOTICS_ROBOT_COMM_RECEIVE_H

#include <winsock.h>
#pragma comment( lib,"ws2_32.lib" )

#include <stdio.h>

namespace cu
{
  namespace robotics
  {
    class RobotCommReceive
    {
    public:
      static const int MaxArraySize = 16;  // = 2 doubles

      RobotCommReceive();
      ~RobotCommReceive();

      void initialize(const unsigned short port);
      void receive(double UDP_DATA[2]);

    protected:
      void unpack_UDP(double UDP_DATA[2]);

    private:
      // UDP message
      char mUDP_buffer_in[MaxArraySize];

      // UDP socket stuff
      WSADATA wsaData;
      SOCKET hSocket;
      sockaddr_in clientAddr;
      SOCKET recvSocket;
      sockaddr_in SenderAddr;
      int SenderAddrSize;
    };
  }
}

#endif
