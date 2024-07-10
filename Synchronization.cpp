#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include "Node.h"

extern std::unordered_map<unsigned int, Node> nodes;

void receiveMessages(unsigned int uid) {
    Node& node = nodes.at(uid);
    while (true) {
        std::unique_lock<std::mutex> lock(node.mtx);
        node.cv.wait(lock, [&node] { return !node.messageQueue.empty(); });

        while (!node.messageQueue.empty()) {
            Message msg = node.messageQueue.front();
            node.messageQueue.pop();
            if (msg.round == node.currentRound) {
                std::cout << "Node " << node.UID << " received message: " << msg.content << std::endl;
            } else {
                node.messageQueue.push(msg);
            }
        }

        lock.unlock();
        node.cv.notify_all();
    }
}

void synchronizeRounds() {
    for (auto& pair : nodes) {
        Node& node = pair.second;
        std::unique_lock<std::mutex> lock(node.mtx);
        node.cv.wait(lock, [&node] { return node.ready; });
    }

    while (true) {
        for (auto& pair : nodes) {
            Node& node = pair.second;
            node.currentRound++;
            node.cv.notify_all();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void broadcastPulseMessage(unsigned int nodeUID) {
    Node& node = nodes.at(nodeUID);
    for (unsigned int neighborUID : node.neighbors) {
        Node& neighbor = nodes.at(neighborUID);

        Message msg;
        msg.round = node.currentRound;
        msg.content = "Pulse from node " + std::to_string(nodeUID);

        std::unique_lock<std::mutex> lock(neighbor.mtx);
        neighbor.messageQueue.push(msg);
        lock.unlock();
        neighbor.cv.notify_all();
    }
}
