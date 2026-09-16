#pragma once

#include <array>
#include <cstdint>

class RIOT {

    public:
        RIOT();

        void reset();

        // Accès à la RAM interne (128 octets, adresses 0x80-0xFF sur le bus).
        void write(uint16_t address, uint8_t value);
        uint8_t read(uint16_t address);

        // Accès aux registres d'E/S et timer (adresses 0x280-0x29F sur le
        // bus ; reg = address - 0x280).
        void writeIO(uint8_t reg, uint8_t value);
        uint8_t readIO(uint8_t reg);

        // A appeler une fois par cycle CPU (pas par cycle couleur TIA) pour
        // faire progresser le timer.
        void tick();

        // A appeler depuis la gestion des entrées (clavier/manette) pour
        // mettre à jour l'état vu par le jeu. Un bit à 1 = relâché/non
        // pressé (logique active à l'état bas, comme sur le vrai matériel).
        // SWCHA : D7-D4 = manette droite (haut,bas,gauche,droite),
        //         D3-D0 = manette gauche (haut,bas,gauche,droite).
        void setSwitchA(uint8_t value) { switchA = value; }
        // SWCHB : D0 = Reset (0=pressé), D1 = Select (0=pressé),
        //         D3 = Couleur/N&B (1=couleur), D6/D7 = difficulté G/D.
        void setSwitchB(uint8_t value) { switchB = value; }

        std::array<uint8_t, 128> ram;

    private:
        uint8_t timerValue;  // valeur courante du compte à rebours (INTIM)
        int intervalCycles;  // 1, 8, 64 ou 1024 cycles CPU par décrément
        int cyclesAccum;     // accumulateur avant le prochain décrément
        bool expired;        // vrai une fois le timer passé sous 0 (décrémente alors à 1 cycle/tick)
        bool timerFlag;      // bit 7 de TIMINT/INSTAT : mis à 1 à l'expiration, effacé à la lecture de INTIM

        uint8_t switchA = 0xFF; // rien de pressé par défaut
        uint8_t switchB = 0xFF; // rien de pressé par défaut
};
