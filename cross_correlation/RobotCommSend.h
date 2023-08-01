#ifndef CU_ROBOTICS_ROBOT_COMM_SEND_H
#define CU_ROBOTICS_ROBOT_COMM_SEND_H

#include <winsock.h>
#pragma comment( lib,"ws2_32.lib" )

#include <stdio.h>

namespace cu
{
  namespace robotics
  {
    class RobotCommSend
    {
    public:
      RobotCommSend();
      ~RobotCommSend();

      void initialize(const char* ip_addr, const unsigned short port);

      // The message to pack and send over UDP
      void send(const double UDP_DATA[5]);

    protected:
      void pack_UDP(const double UDP_DATA[5]);

    private:
      // UDP message
      char mUDP_buffer_out[40];

      // UDP socket stuff
      WSADATA wsaData;
      SOCKET hSocket;
      sockaddr_in serverAddr;
      SOCKET recvSocket;
      sockaddr_in SenderAddr;
    };
  }
}

#endif
