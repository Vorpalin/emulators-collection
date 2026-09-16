#include "riot.hh"
 
RIOT::RIOT()
  : timerValue(0)
  , intervalCycles(1024)
  , cyclesAccum(0)
  , expired(false)
  , timerFlag(false)
{
    this->reset();
}
 
void RIOT::reset() {
    for (auto& byte : ram) {
        byte = 0; // Initialize all RAM bytes to 0
    }
 
    timerValue = 0;
    intervalCycles = 1024;
    cyclesAccum = 0;
    expired = false;
    timerFlag = false;
 
    switchA = 0xFF;
    switchB = 0xFF;
}
 
void RIOT::write(uint16_t address, uint8_t value) {
    if (address < ram.size()) {
        ram[address] = value; // Write the value to the specified address
    }
}
 
uint8_t RIOT::read(uint16_t address) {
    if (address < ram.size()) {
        return ram[address]; // Return the value at the specified address
    } else {
        return 0xFF; // Return 0xFF for out-of-bounds addresses
    }
}
 
void RIOT::writeIO(uint8_t reg, uint8_t value) {
    switch (reg) {
    case 0x14: // TIM1T : décrémente de 1 toutes les 1 cycle CPU
        intervalCycles = 1;
        timerValue = value;
        cyclesAccum = 0;
        expired = false;
        timerFlag = false;
        break;
    case 0x15: // TIM8T : décrémente de 1 tous les 8 cycles CPU
        intervalCycles = 8;
        timerValue = value;
        cyclesAccum = 0;
        expired = false;
        timerFlag = false;
        break;
    case 0x16: // TIM64T : décrémente de 1 tous les 64 cycles CPU
        intervalCycles = 64;
        timerValue = value;
        cyclesAccum = 0;
        expired = false;
        timerFlag = false;
        break;
    case 0x17: // T1024T : décrémente de 1 tous les 1024 cycles CPU
        intervalCycles = 1024;
        timerValue = value;
        cyclesAccum = 0;
        expired = false;
        timerFlag = false;
        break;
    default:
        // SWACNT, SWBCNT, etc. : ports de direction, non simulés (aucun
        // périphérique réellement câblé dessus dans cet émulateur).
        break;
    }
}
 
uint8_t RIOT::readIO(uint8_t reg) {
    switch (reg) {
    case 0x00: // SWCHA : joysticks
        return switchA;
    case 0x02: // SWCHB : interrupteurs console
        return switchB;
    case 0x04: // INTIM
        timerFlag = false; // la lecture de INTIM efface le flag d'expiration
        return timerValue;
    case 0x05: // TIMINT / INSTAT
        return timerFlag ? 0x80 : 0x00;
    default:
        return 0xFF;
    }
}
 
void RIOT::tick() {
    if (!expired) {
        ++cyclesAccum;
        if (cyclesAccum >= intervalCycles) {
            cyclesAccum = 0;
            if (timerValue == 0) {
                // Le compteur vient de passer sous zéro : à partir de
                // maintenant il décrémente à raison de 1 par cycle CPU,
                // et le flag d'expiration est levé.
                expired = true;
                timerFlag = true;
                timerValue = 0xFF;
            } else {
                --timerValue;
            }
        }
    } else {
        --timerValue; // boucle naturellement sur 8 bits (0x00 -> 0xFF)
    }
}
 