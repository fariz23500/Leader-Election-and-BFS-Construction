#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <queue>

struct Message {
    int round;
    std::string content;
};

struct Triplet {
    unsigned int uid_max;
    unsigned int max_distance;
    unsigned int max_distance_uid;

    Triplet() : uid_max(0), max_distance(0), max_distance_uid(0) {}
    Triplet(unsigned int um, unsigned int md, unsigned int mdu) : uid_max(um), max_distance(md), max_distance_uid(mdu) {}
};

struct Node {
    unsigned int UID;
    std::string hostName;
    unsigned short listeningPort;
    std::vector<unsigned int> neighbors;

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    std::queue<Message> messageQueue;
    int currentRound = 0;
    unsigned int parent = 0;
    std::vector<unsigned int> children;

    // New members for socket handling
    int listeningSocket;
    std::vector<int> neighborSockets;

    Triplet triplet;

    Node() : UID(0), listeningPort(0), listeningSocket(-1), parent(0) {}

    Node(unsigned int uid, const std::string& hostname, unsigned short port)
        : UID(uid), hostName(hostname), listeningPort(port), listeningSocket(-1), triplet(uid, 0, 0) {}

    Node(const Node& other)
        : UID(other.UID), hostName(other.hostName), listeningPort(other.listeningPort),
          neighbors(other.neighbors), ready(other.ready), currentRound(other.currentRound),
          parent(other.parent), children(other.children), listeningSocket(other.listeningSocket), triplet(other.triplet) {}

    Node& operator=(const Node& other) {
        if (this != &other) {
            UID = other.UID;
            hostName = other.hostName;
            listeningPort = other.listeningPort;
            neighbors = other.neighbors;
            ready = other.ready;
            currentRound = other.currentRound;
            parent = other.parent;
            children = other.children;
            listeningSocket = other.listeningSocket;
            triplet = other.triplet;
        }
        return *this;
    }
};

#endif // NODE_H
