#include "RobotCommSend.h"
#include <time.h>

namespace cu
{
  namespace robotics
  {
    //--------------------------------------------------------------
    RobotCommSend::RobotCommSend()
    {
      WSAStartup(MAKEWORD(2,2), &wsaData);
    }

    //--------------------------------------------------------------
    RobotCommSend::~RobotCommSend()
    {
    }

    //--------------------------------------------------------------
    void RobotCommSend::initialize(const char* ip_addr, const unsigned short port)
    {
      hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_port = htons(port);  // 25000
      serverAddr.sin_addr.s_addr = inet_addr(ip_addr);  // 192.168.1.245
      recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      int SenderAddrSize = sizeof(SenderAddr);
    }

    //--------------------------------------------------------------
    void RobotCommSend::send(const double UDP_DATA[5])
    {
      pack_UDP(UDP_DATA);
      sendto(hSocket, mUDP_buffer_out, 5*sizeof(double), 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    }

    //--------------------------------------------------------------
    void RobotCommSend::pack_UDP(const double UDP_DATA[5])
    {
        // zero out the buffer
        memset(mUDP_buffer_out, 0, 5*sizeof(double));

        for (int i = 0; i <= sizeof(UDP_DATA); i++)
        {
            memcpy(mUDP_buffer_out+(i*sizeof(double)), &(UDP_DATA[i]), sizeof(double));
        }
    }
  }
}
