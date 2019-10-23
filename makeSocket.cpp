#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <memory>
#include <vector>
#include <algorithm>

using namespace std;

using ConnInfo = struct ConnectionInfo {
    ConnectionInfo() : _sockfd(-1)
                     , _port(-1)
                     , _valid(false) 
    {
    }
    ConnectionInfo(int s, int p, bool v) : _sockfd(s)
                                         , _port(p)
                                         , _valid(v)
    {
    }
    int _sockfd;
    int _port;
    bool _valid;
};

using ConnInfoList = std::vector<ConnInfo>;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

ConnInfoList makeListenSocket(int portno, int maxSocketCount)
{
    int sockfd = -1;
    auto connectionList = ConnInfoList(1024);
    printf("[DBG_INFO] maxSocketCount = %d\n", maxSocketCount);
    for (int i = 0; i < maxSocketCount; ++i) {
        struct sockaddr_in serv_addr;

        // create a socket
        sockfd =  socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("ERROR opening socket");
        }

        // clear address structure
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;  

        serv_addr.sin_addr.s_addr = INADDR_ANY;  

        serv_addr.sin_port = htons(portno);

        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            error("ERROR on binding");
        }

        printf("[sockfd=%5d] listen for (portno=%d)\n", sockfd, portno);
        listen(sockfd, 5);
        connectionList.emplace_back(sockfd, portno++, true);
    }
    return connectionList;
}

int main(int argc, char *argv[])
{
     int sockfd = -1;
     int portno = -1;
     int socketCount = 1; 
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in cli_addr;
     int n;
     
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     if (argc == 3) {
         socketCount = atoi(argv[2]);
     }


     portno = atoi(argv[1]);
     
     auto connections = makeListenSocket(portno, socketCount);


     for (auto& ci : connections) {
         if (!ci._valid) {
             continue;
         }
         printf("continue?? (ESC for quit) \n");
         const char code = getchar();
         if (code == 27) { // exit when esc key entered.
             break;
         }

         clilen = sizeof(cli_addr);

         printf("accepting...\n");
         int newsockfd = accept(ci._sockfd, (struct sockaddr *)&cli_addr, &clilen);
         if (newsockfd < 0) {
            error("ERROR on accepting ");
         }

         printf("server: got connection from %s port %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));


         send(newsockfd, "Hello, world!\n", 13, 0);

         bzero(buffer,256);

         n = read(newsockfd,buffer,255);
         if (n < 0) error("ERROR reading from socket");
         printf("Here is the message: %s\n",buffer);

         close(newsockfd);
         close(ci._sockfd);
         ci._valid = false;
         printf("waiting connection\n");
         sleep(100);
     }


     {
         auto start = connections.begin();
         auto end = connections.end();
         std::for_each(start, end, [](auto& ci) { if (ci._valid) close(ci._sockfd); });
     }

     return 0; 
}
