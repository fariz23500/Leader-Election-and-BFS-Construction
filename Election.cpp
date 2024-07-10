#include "Node.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <cerrno>
#include <thread>

extern std::unordered_map<unsigned int, Node> nodes;

void sendTriplet(Node &node, const Triplet &triplet, int sock)
{
    std::string message = std::to_string(triplet.uid_max) + " " +
                          std::to_string(triplet.max_distance) + " " +
                          std::to_string(triplet.max_distance_uid);
    int retries = 3;

    while (retries > 0)
    {
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval timeout;
        timeout.tv_sec = 4;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, nullptr, &writefds, nullptr, &timeout);

        if (activity > 0 && FD_ISSET(sock, &writefds))
        {
            int sent = send(sock, message.c_str(), message.length(), 0);
            if (sent != -1)
            {
                std::cout << "Node " << node.UID << " sent triplet: " << message << " to socket " << sock << std::endl;
                return;
            }
        }
        std::cerr << "Node " << node.UID << " failed to send triplet to socket " << sock << " with error: " << strerror(errno) << std::endl;
        retries--;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool receiveTriplet(Node &node, Triplet &triplet, int sock)
{
    char buffer[1024];
    int retries = 3;

    while (retries > 0)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 4;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

        if (activity > 0 && FD_ISSET(sock, &readfds))
        {
            int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
            if (bytesRead > 0)
            {
                std::istringstream iss(std::string(buffer, bytesRead));
                iss >> triplet.uid_max >> triplet.max_distance >> triplet.max_distance_uid;
                std::cout << "Node " << node.UID << " received triplet: " << triplet.uid_max << " " << triplet.max_distance << " " << triplet.max_distance_uid << " from socket " << sock << std::endl;
                return true;
            }
            else if (bytesRead == 0)
            {
                std::cerr << "Node " << node.UID << " connection closed by peer on socket " << sock << std::endl;
                return false;
            }
            else
            {
                std::cerr << "Node " << node.UID << " failed to receive triplet from socket " << sock << " with error: " << strerror(errno) << std::endl;
            }
        }
        retries--;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

void pelegsLeaderElection()
{
    int stableRounds = 0;
    const int MAX_ROUNDS = 100; // To avoid infinite loops
    while (stableRounds < 3 && stableRounds < MAX_ROUNDS)
    {
        bool changed = false;
        std::cout << "Starting a new round of leader election" << std::endl;

        // Each node sends its triplet to all its neighbors
        for (auto &pair : nodes)
        {
            Node &node = pair.second;
            std::cout << "Node " << node.UID << " is sending triplets to neighbors" << std::endl;
            for (int sock : node.neighborSockets)
            {
                sendTriplet(node, node.triplet, sock);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // Each node receives triplets from all its neighbors
        for (auto &pair : nodes)
        {
            Node &node = pair.second;
            std::vector<Triplet> receivedTriplets;

            fd_set readfds;
            FD_ZERO(&readfds);
            int max_fd = -1;

            for (int sock : node.neighborSockets)
            {
                FD_SET(sock, &readfds);
                if (sock > max_fd)
                {
                    max_fd = sock;
                }
            }

            struct timeval timeout;
            timeout.tv_sec = 4; // 4 seconds timeout for receiving triplets
            timeout.tv_usec = 0;

            int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);

            if (activity > 0)
            {
                for (int sock : node.neighborSockets)
                {
                    if (FD_ISSET(sock, &readfds))
                    {
                        Triplet triplet;
                        if (receiveTriplet(node, triplet, sock))
                        {
                            receivedTriplets.push_back(triplet);
                        }
                    }
                }
            }
            else if (activity == 0)
            {
                std::cerr << "Node " << node.UID << " timed out waiting for triplets" << std::endl;
            }
            else
            {
                std::cerr << "Node " << node.UID << " error in select()" << std::endl;
            }

            // Save the current triplet to check for changes
            Triplet oldTriplet = node.triplet;

            // Update node's triplet based on received triplets
            std::cout << "Node " << node.UID << " is updating its triplet" << std::endl;
            for (const Triplet &t : receivedTriplets)
            {
                if (t.uid_max > node.triplet.uid_max)
                {
                    node.triplet.uid_max = t.uid_max;
                    node.triplet.max_distance = t.max_distance + 1;
                    node.triplet.max_distance_uid = std::max(t.max_distance + 1, t.max_distance_uid);
                }
                else if (t.uid_max == node.triplet.uid_max)
                {
                    node.triplet.max_distance_uid = std::max(node.triplet.max_distance_uid, t.max_distance_uid);
                }
            }

            // Check if there was any change in the triplet
            if (oldTriplet.uid_max != node.triplet.uid_max ||
                oldTriplet.max_distance != node.triplet.max_distance ||
                oldTriplet.max_distance_uid != node.triplet.max_distance_uid)
            {
                changed = true;
                std::cout << "Node " << node.UID << " triplet changed to: " << node.triplet.uid_max << " " << node.triplet.max_distance << " " << node.triplet.max_distance_uid << std::endl;
            }
        }

        // Check if all triplets have been stable for the required number of rounds
        if (!changed)
        {
            stableRounds++;
            std::cout << "No changes in this round, stableRounds: " << stableRounds << std::endl;
        }
        else
        {
            stableRounds = 0;
            std::cout << "Changes detected, resetting stableRounds to 0" << std::endl;
        }
    }

    if (stableRounds == MAX_ROUNDS)
    {
        std::cerr << "Leader election did not stabilize within the maximum rounds. Potential issue detected." << std::endl;
    }

    // Output the elected leader
    unsigned int leaderUID = nodes.begin()->second.triplet.uid_max;
    std::cout << "Leader elected: Node " << leaderUID << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void sendBFSMessage(Node &node, const std::string &message, int sock)
{
    for (int i = 0; i < 3; ++i)
    { // Retry 3 times
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, nullptr, &writefds, nullptr, &timeout);

        if (activity > 0 && FD_ISSET(sock, &writefds))
        {
            if (send(sock, message.c_str(), message.length(), 0) != -1)
            {
                std::cout << "Node " << node.UID << " sent BFS message: " << message << " to socket " << sock << std::endl;
                return;
            }
        }
        std::cerr << "Node " << node.UID << " failed to send BFS message to socket " << sock << " with error: " << strerror(errno) << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before retrying
    }
}

bool receiveBFSMessage(Node &node, std::string &message, int sock)
{
    char buffer[1024];
    for (int i = 0; i < 3; ++i)
    { // Retry 3 times
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

        if (activity > 0 && FD_ISSET(sock, &readfds))
        {
            int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
            if (bytesRead > 0)
            {
                message = std::string(buffer, bytesRead);
                std::cout << "Node " << node.UID << " received BFS message: " << message << " from socket " << sock << std::endl;
                return true;
            }
            else if (bytesRead == 0)
            {
                std::cerr << "Node " << node.UID << " connection closed by peer on socket " << sock << std::endl;
                return false;
            }
            else
            {
                std::cerr << "Node " << node.UID << " failed to receive BFS message from socket " << sock << " with error: " << strerror(errno) << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before retrying
    }
    return false;
}

void bfsTreeConstruction(unsigned int leaderUID)
{
    Node &leader = nodes.at(leaderUID);
    leader.parent = leaderUID;

    std::queue<unsigned int> bfsQueue;
    bfsQueue.push(leaderUID);

    std::cout << "BFS tree construction started with leader: Node " << leaderUID << std::endl;

    // BFS tree construction
    while (!bfsQueue.empty())
    {
        unsigned int currentUID = bfsQueue.front();
        bfsQueue.pop();
        Node &currentNode = nodes.at(currentUID);

        for (unsigned int neighborUID : currentNode.neighbors)
        {
            Node &neighborNode = nodes.at(neighborUID);
            if (neighborNode.parent == 0)
            {
                neighborNode.parent = currentUID;
                currentNode.children.push_back(neighborUID);
                bfsQueue.push(neighborUID);
            }
        }
    }

    // Output the BFS tree
    for (const auto &pair : nodes)
    {
        const Node &node = pair.second;
        std::cout << "Node " << node.UID << " - Parent: " << node.parent << ", Children: ";
        for (unsigned int child : node.children)
        {
            std::cout << child << " ";
        }
        std::cout << std::endl;
    }
}
