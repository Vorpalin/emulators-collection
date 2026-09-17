#include "atari2600_bus.hh"
#include <algorithm>
#include <cstddef>
#include <stdexcept>


Atari2600Bus::Atari2600Bus(CPU6507* cpu) {
    this->cpu = cpu;
    this->tia = new TIA();
    this->riot = new RIOT();
    this->cartridge = new CartridgeAtari2600();
}

Atari2600Bus::~Atari2600Bus() {
    delete this->tia;
    delete this->riot;
    delete this->cartridge;
}

uint8_t Atari2600Bus::read(uint16_t address) {
    address &= 0x1FFF;

    if (address >= 0x1000) {
        if (this->cartridge->size() == 0) {
            return 0xFF;
        }
        return this->cartridge->read((address - 0x1000) % this->cartridge->size());
    } else if (address < 0x0080) {
        return this->tia->read(address & 0x3F);
    } else if (address >= 0x0080 && address < 0x0100) {
        return this->riot->read(address - 0x0080);
    } else if (address >= 0x0280 && address < 0x02A0) {
        return this->riot->readIO(static_cast<uint8_t>(address - 0x0280));
    } else {
        return this->cpu->memory[address];
    }
}

void Atari2600Bus::loadCartridge(const std::string& path) {
    if (!this->cartridge->load(path)) {
        throw std::runtime_error("Failed to load cartridge: " + path);
    }

    this->cpu->reset();
    this->tia->reset();
    this->riot->reset();
}

void Atari2600Bus::write(uint16_t address, uint8_t value) {
    address &= 0x1FFF;

    if (address >= 0x1000) {
        return;
    } else if (address < 0x0080) {
        this->tia->write(address & 0x3F, value);
    } else if (address >= 0x0080 && address < 0x0100) {
        this->riot->write(address - 0x0080, value);
    } else if (address >= 0x0280 && address < 0x02A0) {
        this->riot->writeIO(static_cast<uint8_t>(address - 0x0280), value);
    } else {
        this->cpu->memory[address] = value;
    }
}

void Atari2600Bus::tick() {
    if (this->tia->isWsyncPending()) {
        // Le CPU est à l'arrêt (WSYNC), mais l'horloge système continue de
        // tourner : sur le vrai matériel, le timer du RIOT continue de
        // décompter pendant ce temps-là, il ne se fige pas.
        this->riot->tick();
        this->tia->tick();
        this->tia->tick();
        this->tia->tick();
    } else {
        // Séquencement cycle-exact : on connaît la durée totale de la
        // PROCHAINE instruction avant de l'exécuter (peekCycles), ce qui
        // permet de faire avancer le TIA/RIOT du temps correspondant aux
        // cycles de "préparation" (fetch de l'opcode, de l'opérande, etc.)
        // AVANT que les effets de bord de l'instruction (écritures dans
        // les registres TIA/RIOT : couleurs, playfield, GRPx, HMOVE,
        // WSYNC, RESPx...) ne soient appliqués par step().
        //
        // Sur le vrai 6507, une écriture (STA, INC, ...) a lieu au tout
        // dernier cycle de l'instruction, jamais avant : c'est ce qu'on
        // reproduit ici en réservant explicitement le dernier cycle pour
        // après l'appel à step().
        const uint8_t totalCycles = this->cpu->peekCycles();
        const uint8_t setupCycles = (totalCycles > 0) ? (totalCycles - 1) : 0;

        for (uint8_t i = 0; i < setupCycles; ++i) {
            this->riot->tick();
            this->tia->tick();
            this->tia->tick();
            this->tia->tick();
        }

        this->cpu->step(); // Les écritures bus de l'instruction sont appliquées ici, "au dernier cycle"

        this->riot->tick();
        this->tia->tick();
        this->tia->tick();
        this->tia->tick();
    }

    if (this->tia->isFrameReady()) {
        this->tia->renderFrame();
    }
}

void Atari2600Bus::setRenderer(SDL_Renderer* renderer) {
    this->tia->setRenderer(renderer);
}