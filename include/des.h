#pragma once

#include <vector>
#include <cstdint>
#include <string>

class DES {
public:
    static const int BLOCK_SIZE = 8;
    static const int KEY_SIZE = 8;
    
    DES(const uint8_t* key);
    ~DES();
    
    std::vector<uint8_t> encrypt(const uint8_t* data, size_t len);
    std::vector<uint8_t> decrypt(const uint8_t* data, size_t len);
    
    // VNC-specific DES encryption with key reversal
    static std::vector<uint8_t> vnc_encrypt(const std::string& password, const uint8_t* challenge);
    
private:
    uint8_t key[8];
    uint32_t subkeys[16][48];
    
    void generateSubkeys();
    void permute(const uint8_t* input, uint8_t* output, const int* table, int size);
    void feistel(uint32_t& left, uint32_t& right, int round);
    uint32_t sbox(uint8_t input, int box);
    void processBlock(const uint8_t* input, uint8_t* output, bool encrypt);
};

