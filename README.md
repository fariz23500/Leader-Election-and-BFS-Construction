# Leader-Election-and-BFS-Construction
Pelegs leader election algorithm and BFS tree constructed using socket programming

• Run g++ -std=c++11 Node.h Utils.h  ConfigParser.cpp Utils.cpp  SCTPHelper.cpp 
Election.cpp Synchronization.cpp  Initialization.cpp -lsctp -lpthread and ./a.out 
.On all other servers except one main one to initialize all the nodes on their 
servers. 
• Run g++ -std=c++11 Node.h Utils.h  ConfigParser.cpp Utils.cpp  SCTPHelper.cpp 
Election.cpp Synchronization.cpp  server.cpp -lsctp -lpthread and ./a.out . On 
the main server to initialize a node and run Pelegs leader election an BFS tree 
construction algorithm. 


