#include <array>
#include <cstdint>

class RIOT {

    public:
        RIOT();

        void reset();
        std::array<uint8_t, 128> ram;


};