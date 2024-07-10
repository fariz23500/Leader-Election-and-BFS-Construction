#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <csignal>
#include <unistd.h>
#include "Node.h"
#include <algorithm>

std::unordered_map<unsigned int, Node> nodes;

void parseConfigurationFile(const std::string &filename);
void initSockets();
void startServer(unsigned int uid);
void connectNodes();
void pelegsLeaderElection();
void bfsTreeConstruction(unsigned int leaderUID);
void receiveMessages(unsigned int uid);
void synchronizeRounds();
void broadcastPulseMessage(unsigned int nodeUID);
void cleanup(int signum);

std::string getLocalIPAddress()
{
    // This function should return the local IP address of the machine
    char buffer[128];
    std::string ipAddress = "127.0.0.1"; // default to localhost
    // FILE *pipe = popen("hostname -I", "r");
    // if (pipe)
    // {
    //     while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    //     {
    //         ipAddress = buffer;
    //         break;
    //     }
    //     pclose(pipe);
    // }
    // // Trim the IP address to remove any trailing spaces or newlines
    // ipAddress.erase(std::remove_if(ipAddress.begin(), ipAddress.end(), ::isspace), ipAddress.end());
    return ipAddress;
}

int main()
{
    signal(SIGINT, cleanup); // Register signal handler for cleanup on exit
    std::string localIPAddress = getLocalIPAddress();
    std::cout << localIPAddress << " ";
    std::cout << "Parsing configuration file..." << std::endl;
    parseConfigurationFile("config.txt");

    std::cout << "Initializing sockets..." << std::endl;
    initSockets();

    std::cout << "Starting servers..." << std::endl;
    std::vector<std::thread> serverThreads;
    for (const auto &pair : nodes)
    {
        if (pair.second.hostName == localIPAddress)
        {
            serverThreads.push_back(std::thread(startServer, pair.second.UID));
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Connecting nodes..." << std::endl;
    connectNodes();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Starting leader election..." << std::endl;
    pelegsLeaderElection();

    unsigned int leaderUID = nodes.begin()->second.triplet.uid_max;
    std::cout << "Leader elected: Node " << leaderUID << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Starting BFS tree construction..." << std::endl;
    bfsTreeConstruction(leaderUID);

    // std::cout << "Starting message receivers..." << std::endl;
    // std::vector<std::thread> receiverThreads;
    // for (const auto& pair : nodes) {
    //     receiverThreads.push_back(std::thread(receiveMessages, pair.second.UID));
    // }

    // std::cout << "Starting synchronization thread..." << std::endl;
    // std::thread syncThread(synchronizeRounds);

    // while (true) {
    //     std::this_thread::sleep_for(std::chrono::seconds(5));
    //     unsigned int nodeToBroadcast = 5;
    //     broadcastPulseMessage(nodeToBroadcast);
    // }

    // for (auto& thread : serverThreads) {
    //     if (thread.joinable()) {
    //         thread.join();
    //     }
    // }

    // for (auto& thread : receiverThreads) {
    //     if (thread.joinable()) {
    //         thread.join();
    //     }
    // }

    // if (syncThread.joinable()) {
    //     syncThread.join();
    // }

    return 0;
}

void cleanup(int signum)
{
    std::cout << "Cleaning up sockets..." << std::endl;
    for (auto &pair : nodes)
    {
        Node &node = pair.second;
        if (node.listeningSocket != -1)
        {
            close(node.listeningSocket);
        }
        for (int sock : node.neighborSockets)
        {
            close(sock);
        }
    }
    exit(signum);
}