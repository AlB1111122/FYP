#include <iostream>

#include <bitset>

void printMemoryAsBits(void* ptr, size_t size) {
    unsigned char* bytePtr = static_cast<unsigned char*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        // Print each byte as a bitset
        std::bitset<8> bits(bytePtr[i]);
        std::cout << bits;
    }
    std::cout << std::endl;
}

int main(){
    uint8_t arr[9]={1,2,3,1,2,3,1,2,3};
    printMemoryAsBits(arr, sizeof(arr));
    return 0;
}