#pragma once

#include <cstdint>
#include <array>

class MOS6532 {
    // Class definition for the MOS6532 (Random Information and Output) component
    public:
        MOS6532();

        void reset();

        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);

        void update(int cpu_cycles);

        void setSwcha(uint8_t v) { io_ports[0] = v; }
        void setSwchb(uint8_t v) { io_ports[1] = v; }
    private:
        std::array<uint8_t, 128> ram; // 128 bytes of RAM
        std::array<uint8_t, 2> io_ports;  // Ports d'E/S (Port A et Port B)
        std::array<uint8_t, 2> ddrs;      // Data Direction Registers (porta, portb)

        // Gestion du Timer
        uint32_t timer_counter = 0;       // Compteur interne en cycles CPU
        int timer_interval = 1;           // Facteur de division (1, 8, 64, ou 1024)

        bool timer_expired = false;

        void set_timer(uint8_t value, int interval);
        uint8_t get_timer_value();
};