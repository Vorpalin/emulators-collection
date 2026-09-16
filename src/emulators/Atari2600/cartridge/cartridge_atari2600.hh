#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

class CartridgeAtari2600 {
public:
    bool load(const std::string& path);

    uint8_t read(uint16_t address);
    void reset();
    std::size_t size() const;

    std::vector<uint8_t> getROM() const {
        return rom;
    }

private:
    std::vector<uint8_t> rom;
};