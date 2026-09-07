#include <cstdint>
#include <vector>
#include <string>

class CartridgeAtari2600 {
public:
    bool load(const std::string& path);

    uint8_t read(uint16_t address);

private:
    std::vector<uint8_t> rom;
};