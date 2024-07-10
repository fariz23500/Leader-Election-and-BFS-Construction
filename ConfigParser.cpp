#include <iostream>
#include <fstream>
#include <sstream>
#include "Node.h"
#include <algorithm>
extern std::unordered_map<unsigned int, Node> nodes;

void parseConfigurationFile(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile) {
        std::cerr << "Failed to open configuration file." << std::endl;
        return;
    }

    std::string line;
    int nodeCount = 0;
    bool readingNodes = false;
    bool readingNeighbors = false;
    std::vector<unsigned int> nodeOrder;

    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        if (!readingNodes) {
            if (!(iss >> nodeCount)) continue;
            readingNodes = true;
            std::cout << "Number of nodes: " << nodeCount << std::endl;
        } else if (nodeCount > 0) {
            unsigned int uid;
            std::string hostname;
            unsigned short port;
            if (!(iss >> uid >> hostname >> port)) continue;
            nodes.emplace(std::piecewise_construct,
                          std::forward_as_tuple(uid),
                          std::forward_as_tuple(uid, hostname, port));
            nodeOrder.push_back(uid);
            std::cout << "Added node: " << uid << " with hostname: " << hostname << " and port: " << port << std::endl;
            nodeCount--;
            if (nodeCount == 0) readingNeighbors = true;
        } else if (readingNeighbors) {
            static int currentNodeIndex = 0;
            if (currentNodeIndex >= nodeOrder.size()) break;
            unsigned int currentNodeUID = nodeOrder[currentNodeIndex];
            Node& currentNode = nodes.at(currentNodeUID);
            unsigned int neighbor;
            // std::cout << "Reading neighbors for node: " << currentNodeUID << std::endl;
            while (iss >> neighbor) {
                if (std::find(currentNode.neighbors.begin(), currentNode.neighbors.end(), neighbor) == currentNode.neighbors.end()) {
                    currentNode.neighbors.push_back(neighbor);
                    // std::cout << "Added neighbor: " << neighbor << " to node: " << currentNodeUID << std::endl;
                }
            }
            currentNodeIndex++;
        }
    }

    // Debug output to verify nodes and neighbors
    for (const auto& pair : nodes) {
        const Node& node = pair.second;
        std::cout << "Node " << node.UID << " - Neighbors: ";
        for (unsigned int neighbor : node.neighbors) {
            std::cout << neighbor << " ";
        }
        std::cout << std::endl;
    }
}