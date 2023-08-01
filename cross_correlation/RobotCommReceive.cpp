#include "RobotCommReceive.h"
#include <time.h>

namespace cu
{
  namespace robotics
  {
    //--------------------------------------------------------------
    RobotCommReceive::RobotCommReceive()
    {
      WSAStartup(MAKEWORD( 2, 2 ), &wsaData);
    }

    //--------------------------------------------------------------
    RobotCommReceive::~RobotCommReceive()
    {
      closesocket(recvSocket);
    }

    //--------------------------------------------------------------
    void RobotCommReceive::initialize(const unsigned short port)
    {
      SenderAddrSize = sizeof(SenderAddr);
      hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      clientAddr.sin_family = AF_INET;
      clientAddr.sin_port = htons(port);
      clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

      // Bind the socket.
      bind(hSocket, reinterpret_cast<sockaddr *>(&clientAddr), sizeof(clientAddr));
    }

    //--------------------------------------------------------------
    void RobotCommReceive::receive(double UDP_DATA[2])
    {
      int bytesReceived = recvfrom(hSocket, mUDP_buffer_in, MaxArraySize, 0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
      unpack_UDP(UDP_DATA);
    }

    //--------------------------------------------------------------
    void RobotCommReceive::unpack_UDP(double UDP_DATA[2])
    {
      memcpy(&(UDP_DATA[0]), mUDP_buffer_in, sizeof(double));
      memcpy(&(UDP_DATA[1]), mUDP_buffer_in+sizeof(double), sizeof(double));
    }
  }
}

