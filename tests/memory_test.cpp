#include <vector>
#include <iostream>

int main() {
    std::vector<char*> blocks;

    while (true) {
        char* block = new char[10 * 1024 * 1024]; // 10MB
        blocks.push_back(block); // add 10MB block for every cycle
        std::cout << "Allocated 10MB\n";
    }
}
