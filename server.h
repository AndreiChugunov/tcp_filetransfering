//
// Created by 809279 on 10.11.2017.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include <string>
#include <vector>
#include <unordered_map>


namespace ftr {
    

    class Server {
    public:
    	const std::string DEFAULT_PORT = "27022";
	    const std::string BASE_DIR = "/home/user/NetBeansProjects/server/";
	    const int DEFAULT_BUFLEN = 512;
	    const std::string endString = "closeme";
	    const std::string getList = "ls";
	    const std::string getFile = "get";
	    const std::string postFile = "post";
	    const std::string changeDirectory = "cd";
	    const std::string currentDir = "cur";
	    const std::string endServer = "end";
	    const std::string closeClient = "close";
	    const std::string showListOfClients = "show";
	    const std::string begining = "TRANSMITION START";
    	const std::string ending = "TRANSMITION ENDING";

        int start();
        Server();
        void acceptFunction(int ListenSocket);
        void handleFunction(int ClientSocket);
    private:
        std::vector<std::pair<int, int>> poolOfSockets;
        std::unordered_map<int, std::string> eachClientCurrentDir;
        pthread_mutex_t mainThreadMutex;

        int readn(int newsockfd, char *buffer, int n);
        
        void sendfile(std::vector<std::string> vec, int ClientSocket);
        void sendlist(int ClientSocket);
        void changedirectory(int ClientSocket, std::vector<std::string> vec);
        void closeclient(int ClientSocket);
        void getfile(std::vector<std::string> vec, int ClientSocket);
        void sendcurrentclientdir(int ClientSocket, std::vector<std::string> vec);

        void stop(int ListenSocket, pthread_t acceptThread);
        void disconnectClient();
        void showClientList();
    };
    struct threadStruct {
        Server* server;
        int ListenSocket;
    };

}
#endif //SERVER_SERVER_H
