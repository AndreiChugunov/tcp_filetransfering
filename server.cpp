//
// Created by 809279 on 10.11.2017.
//

#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include "server.h"
#include "utils.h"

namespace ftr {

	Server::Server():
	poolOfSockets(),
	eachClientCurrentDir()
	{

	}


	int Server::readn(int newsockfd, char *buffer, int n) {
	    int nLeft = n;
	    int k;
	    while (nLeft > 0) {
	        k = recv(newsockfd, buffer, nLeft, 0);
	        if (k < 0) {
	            perror("ERROR reading from socket");
	            return -1;
	        } else if (k == 0) break;
	        buffer = buffer + k;
	        nLeft = nLeft - k;
	    }
	    return n - nLeft;
	}

	void Server::getfile(std::vector<std::string> vec, int ClientSocket) {
		int readResult;
		char buf[DEFAULT_BUFLEN];

	    readResult = readn(ClientSocket, buf, DEFAULT_BUFLEN);
	    if (readResult <= 0) {
	    	closeclient(ClientSocket);
	    	return;
	    }
        std::string ans(buf);         
        if (ans == begining) {
            readResult = readn(ClientSocket, buf, DEFAULT_BUFLEN);
            if (readResult <= 0) {
	    		closeclient(ClientSocket);
	    		return;
	    	}
            int lastPortionSize = atoi(buf);
            char byteArray[DEFAULT_BUFLEN];

            std::vector<std::string> fileNameVector = parseString((char*)vec[1].c_str(), (char *)"\\");
	    	std::string fileNameString = fileNameVector[fileNameVector.size() - 1];
            
            std::ofstream ofs((eachClientCurrentDir[ClientSocket] + fileNameString).c_str(), std::ios::out | std::ios::binary);
            readResult = readn(ClientSocket, byteArray, DEFAULT_BUFLEN);
            if (readResult <= 0) {
	    		closeclient(ClientSocket);
	    		return;
	    	}
            while(std::string(byteArray) != ending) {
                ofs.write(byteArray, DEFAULT_BUFLEN);
            	readResult = readn(ClientSocket, byteArray, DEFAULT_BUFLEN);
            	if (readResult <= 0) {
	    			closeclient(ClientSocket);
	    			return;
	    		}
            }
            readResult = readn(ClientSocket, byteArray, DEFAULT_BUFLEN);
            if (readResult <= 0) {
	    		closeclient(ClientSocket);
	    		return;
	    	}
            ofs.write(byteArray, lastPortionSize);
            ofs.close();
        }
	}

	void Server::sendcurrentclientdir(int ClientSocket, std::vector<std::string> vec) {
		if (send(ClientSocket, eachClientCurrentDir[ClientSocket].c_str(), DEFAULT_BUFLEN, 0) <= 0) {
			closeclient(ClientSocket);
			return;
		}
		return;
	}

	void Server::changedirectory(int ClientSocket, std::vector<std::string> vec) {
	    DIR *dir;
	    struct dirent *ent;
	    std::string directoryResponse;
	    if (vec[1] == "..") {
	    	std::vector<std::string> parsedDir = parseString(const_cast< char *>(BASE_DIR.c_str()), (char*)"/");
	    	vec[1] = "/";
	    	for (int i = 0; i < parsedDir.size() - 1; i++) {
	    		vec[1] = vec[1] + parsedDir[i] + "/";
	    	}
	    }
	    if (vec[1][0] != '/') 
	    	if ((dir = opendir((eachClientCurrentDir[ClientSocket] + vec[1]).c_str())) != NULL) {
	    		vec[1] = eachClientCurrentDir[ClientSocket] + vec[1];
	    		closedir(dir);
	    	}

	    if ((dir = opendir (vec[1].c_str())) != NULL) {
	        if (vec[1][vec[1].size() - 1] != '/') vec[1] = vec[1] + "/";
	        eachClientCurrentDir[ClientSocket] = vec[1];
	        directoryResponse = "you are now in: " + vec[1];
	        closedir(dir);
	    } else {
	        directoryResponse = "can't cd to this directory";
	    }
	    if (send(ClientSocket, directoryResponse.c_str(), DEFAULT_BUFLEN, 0) <= 0) {
	    	closeclient(ClientSocket);
	    	return;
	    }
	    return;
	}

	void Server::closeclient(int ClientSocket) {
	    shutdown(ClientSocket, 2);
	    close(ClientSocket);
	    int deleteSock = -1;
	    pthread_mutex_lock(&mainThreadMutex);
	    for(int i = 0; i < poolOfSockets.size(); i++) {
	        if (poolOfSockets[i].second == ClientSocket) {
	            deleteSock = i;
	            break;
	        }
	    }
	    if (deleteSock != -1) {
	        poolOfSockets.erase(poolOfSockets.begin() + deleteSock);
	    }
	    pthread_mutex_unlock(&mainThreadMutex);
	}

	void Server::sendfile(std::vector<std::string> vec, int ClientSocket) {
		std::ifstream ifs(eachClientCurrentDir[ClientSocket] + vec[1], std::ios::binary | std::ios::in | std::ios::ate);
    	if(ifs.is_open()) {
	        std::streampos size = ifs.tellg(); 

	        ifs.seekg (0, std::ios::beg);
	        std::streampos DEFAULT_BUFLEN_POS = DEFAULT_BUFLEN;
	        std::streampos lastPortion = size - (size / DEFAULT_BUFLEN_POS) * DEFAULT_BUFLEN_POS;
	        lastPortion = (lastPortion > 0 ? lastPortion : DEFAULT_BUFLEN_POS);

	        if (send(ClientSocket, begining.c_str(), DEFAULT_BUFLEN, 0) == -1) {
	        	closeclient(ClientSocket);
	            return;
	        }

	        if (send(ClientSocket, std::to_string(lastPortion).c_str(), DEFAULT_BUFLEN, 0) == -1) {
	        	closeclient(ClientSocket);
	            return;
	        }

	        char portion[DEFAULT_BUFLEN];
	        while (ifs.tellg() < (size - DEFAULT_BUFLEN_POS)) {
	            ifs.read(portion, DEFAULT_BUFLEN);
	            if (send(ClientSocket, portion, DEFAULT_BUFLEN, 0) == -1) {
	            	closeclient(ClientSocket);
	            	return;
	            }
	        }
			
	        if (send(ClientSocket, ending.c_str(), DEFAULT_BUFLEN, 0) == -1) {
	        	closeclient(ClientSocket);
	        	return;
	        }

	        ifs.read(portion, DEFAULT_BUFLEN);
	        if (send(ClientSocket, portion, DEFAULT_BUFLEN, 0) == -1) {
	            closeclient(ClientSocket);
	            return;
	        }

	        ifs.close();
        } else {
        	std::cout << "Error opening file\n";
        }
	}

	void Server::sendlist(int ClientSocket) {
	    DIR *dir;
	    struct dirent *ent;
	    if ((dir = opendir (eachClientCurrentDir[ClientSocket].c_str())) != NULL) {
	    	if(send(ClientSocket, begining.c_str(), DEFAULT_BUFLEN, 0) == -1) {
	            closeclient(ClientSocket);
	            return;
	        }
	        while ((ent = readdir (dir)) != NULL) {
	            if (*ent->d_name != '.') {
	            	DIR *directory;
	                std::string type;
	                if ((directory = opendir ((eachClientCurrentDir[ClientSocket] + std::string(ent->d_name)).c_str())) == NULL)
	                    type = "FILE: ";
	                else {
	                    type = "DIR: ";
	                    closedir(directory);
	                }
	                if(send(ClientSocket, (type + std::string(ent->d_name)).c_str(), DEFAULT_BUFLEN, 0) == -1) {
	                	closeclient(ClientSocket);
	                	return;
	            	}
	            }
	        }
	        if(send(ClientSocket, ending.c_str(), DEFAULT_BUFLEN, 0) == -1) {
	            closeclient(ClientSocket);
	            return;
	        }
	        closedir (dir);
	    } else {
	        std::cout << "can't open a directory";
	    }
	    return;
	}

	void Server::stop(int ListenSocket, pthread_t acceptThread) {
	    std::cout << "Closing server \n";
	    shutdown(ListenSocket, 2);
	    close(ListenSocket);
	    pthread_join(acceptThread, NULL);
	}

	void Server::disconnectClient() {
	    std::cout << "---------------\nWrite an id of a client to disconnect\n---------------\n";
	    std::string strNum;
	    std::getline(std::cin, strNum);
	    int num = atoi(strNum.c_str());

	    pthread_mutex_lock(&mainThreadMutex);
	    int indexToDelete = -1;
	    for(int i = 0; i < poolOfSockets.size(); i++) {
	        if (poolOfSockets[i].first == num) {
	            indexToDelete = i;
	            break;
	        }
	    }
	    if (indexToDelete != - 1) {
		    shutdown(poolOfSockets[indexToDelete].second, 2);
		    close(poolOfSockets[indexToDelete].second);
		    poolOfSockets.erase(poolOfSockets.begin() + indexToDelete);
		}
	    pthread_mutex_unlock(&mainThreadMutex);
	}

	void Server::showClientList() {
	    std::cout << "Available clients: ";
	    pthread_mutex_lock(&mainThreadMutex);
	    for(int i = 0; i < poolOfSockets.size(); i++) {
	        std::cout << poolOfSockets[i].first << ", ";
	    }
	    std::cout << "\n";
	    pthread_mutex_unlock(&mainThreadMutex);
	}

	void * threadedFunction(void *pArguments) {
	    threadStruct handleThreadStruct = *static_cast< threadStruct* >(pArguments);
	    int ClientSocket = handleThreadStruct.ListenSocket;
	    Server* server = handleThreadStruct.server;
	    server->handleFunction(ClientSocket);
	}

	void Server::handleFunction(int ClientSocket) {

	    int readed = -1;
	    do {
	        
	        char recvbuf[DEFAULT_BUFLEN];
	        readed = readn(ClientSocket, recvbuf, DEFAULT_BUFLEN);
	        if (readed <= 0) break;

	        std::cout << "Bytes read: " << readed << std::endl;
	        std::string str(recvbuf);
	        std::cout<<str<<std::endl;
	        std::vector<std::string> vec = parseString((char *)str.c_str(), (char *)" ");

	        if (vec[0] == getList) sendlist(ClientSocket);
	        if (vec[0] == getFile) sendfile(vec, ClientSocket);
	        if (vec[0] == postFile) getfile(vec, ClientSocket);
	        if (vec[0] == changeDirectory) changedirectory(ClientSocket, vec);
	        if (vec[0] == currentDir) sendcurrentclientdir(ClientSocket, vec);
	        if (vec[0] == endString) break;
	    
	    } while (readed > 0);
	    
	    closeclient(ClientSocket);
	    return;
	}

	void * acceptThreadFunction(void *pArguments) {
	    threadStruct acceptthreadStruct = *(static_cast< threadStruct* >(pArguments));
	    int ListenSocket = acceptthreadStruct.ListenSocket;
	    Server* server = acceptthreadStruct.server;
	    server->acceptFunction(ListenSocket);
	}
	void Server::acceptFunction(int ListenSocket) {

	    std::vector<pthread_t> vectorOfThreads;
	    int connectionsCounter = 0;
	    while(true){
	        int ClientSocket = accept(ListenSocket, NULL, NULL);
	        if (ClientSocket == -1) {
	            std::cout << "accept failed with error: \n";
	            close(ListenSocket);
	            break;
	        }
	        
	        
	        pthread_t thread;
	        threadStruct handleThreadDisc {
	            this,
	            ClientSocket
	    	};
	        pthread_create(&thread, NULL, &threadedFunction, static_cast< void* > (&handleThreadDisc));
	        pthread_mutex_lock(&mainThreadMutex);
	        poolOfSockets.push_back(std::make_pair(connectionsCounter, ClientSocket));
	        eachClientCurrentDir.insert(std::make_pair(ClientSocket, BASE_DIR));
	        vectorOfThreads.push_back(thread);
	        std::cout << "new client connected with id: " << connectionsCounter << std::endl;
	        connectionsCounter++;
	        pthread_mutex_unlock(&mainThreadMutex);
	    }
	    std::cout << "closing all connections. Ending all threads\n";
	    for (int i = 0; i < poolOfSockets.size(); i ++) {
	        shutdown(poolOfSockets[i].second, 2);
	        close(poolOfSockets[i].second);
	    }
	    poolOfSockets.clear();
	    for (int i = 0; i < vectorOfThreads.size(); i++) {
	        pthread_join(vectorOfThreads[i], NULL);
	    }
	    return;
	}

	int Server::start() {
	    int iResult;

	    int ListenSocket = -1;

	    struct addrinfo *result = NULL;
	    struct addrinfo hints;

	    bzero(&hints, sizeof(hints));
	    hints.ai_family = AF_INET;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_protocol = IPPROTO_TCP;
	    hints.ai_flags = AI_PASSIVE;


	    iResult = getaddrinfo(NULL, DEFAULT_PORT.c_str(), &hints, &result);
	    if ( iResult != 0 ) {
	        std::cout << "getaddrinfo failed with error \n";
	        return 1;
	    }

	    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	    if (ListenSocket == -1) {
	        std::cout << "socket failed with error\n";
	        freeaddrinfo(result);
	        return 1;
	    }
	    int yes=1;

	    if (setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
	        perror("setsockopt");
	        exit(1);
	    }
	    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	    if (iResult == -1) {
	        std::cout << "bind failed with error \n";
	        freeaddrinfo(result);
	        close(ListenSocket);
	        return 1;
	    }
	    iResult = listen(ListenSocket, SOMAXCONN);
	    if (iResult == -1) {
	        std::cout << "listen failed with error\n";
	        close(ListenSocket);
	        return 1;
	    }
	    pthread_t acceptThread;
	    threadStruct listenThreadDisc {
	            this,
	            ListenSocket
	    };
	    pthread_create(&acceptThread, NULL, &acceptThreadFunction, static_cast< void* >(&listenThreadDisc));
	    pthread_mutex_init(&mainThreadMutex, NULL);
	    std::string str;

	    while (true) {
	        std::cout << "---------------\nChoose an operation:\n 1) end\n 2) close\n 3) send\n 4) show\n---------------\n";
	        std::getline(std::cin, str);
	        if (str == endServer) {
	            stop(ListenSocket, acceptThread);
	            break;
	        }
	        if (str == closeClient) disconnectClient();
	        if (str == showListOfClients) showClientList();
	    }
	    return 0;
	}
}