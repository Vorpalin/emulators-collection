#pragma once

#include <cstdint>
#include <vector>
#include <SDL2/SDL.h>

class TIA {
public:
    static constexpr int ScreenWidth = 160;
    static constexpr int ScreenHeight = 192;

    TIA();
    ~TIA();

    void write(uint8_t reg, uint8_t value);
    uint8_t read(uint8_t reg);
    void reset();

    void tick();

    // A appeler par le CPU après chaque instruction : tant que ceci renvoie
    // true, le 6507 doit rester en pause (c'est le comportement de WSYNC).
    // Le flag est levé automatiquement par tick() au début de la ligne suivante.
    bool isWsyncPending() const { return wsync; }

    void setRenderer(SDL_Renderer* renderer);
    bool isFrameReady() const;
    void renderFrame();

    // A appeler depuis la gestion des entrées pour mettre à jour l'état
    // des boutons de tir (INPT4 = joueur 0, INPT5 = joueur 1). Bit 7 à 1
    // = relâché, à 0 = pressé (comme sur le vrai matériel). Les paddles
    // (INPT0-INPT3) ne sont pas simulées et renvoient 0xFF.
    void setInputPort(uint8_t port, uint8_t value);

private:
    // Position + registre de mouvement fin (HMxx) d'un objet mobile
    // (joueur, missile ou balle).
    struct MovableObject {
        int position = 0; // 0..ScreenWidth-1, pixel le plus à gauche de l'objet
        uint8_t hm = 0;   // valeur brute du registre HMxx (nibble haut signé)
    };

    void drawPixel(int x, int y);
    void applyHmove();
    void resetPosition(MovableObject& obj);

    bool playfieldPixel(int x) const;
    bool ballPixel(int x) const;
    bool missilePixel(const MovableObject& obj, uint8_t nusiz, bool enabled, int x) const;
    bool playerPixel(const MovableObject& obj, uint8_t grp, uint8_t nusiz, bool reflect, int x) const;

    void updateCollisions(bool pf, bool bl, bool m0, bool m1, bool p0, bool p1);

    uint8_t registers[64];

    // Registres de collision (lecture seule, adresses 0x00-0x07 en lecture).
    // Seuls les bits 6 et 7 sont utilisés, comme sur le vrai TIA.
    uint8_t collisions[8];

    uint16_t scanline; // ligne "faisceau", 0..261 (NTSC)
    uint16_t cycle;    // cycle couleur dans la ligne, 0..227
    int displayLine;   // ligne réellement affichée (0..ScreenHeight-1), pilotée par VBLANK

    bool vsync;
    bool vblank;
    bool wsync;

    uint8_t inputPorts[6]; // INPT0..INPT5 (0xFF = relâché par défaut)

    MovableObject p0, p1, m0, m1, bl;

    // Doubles buffers pour le "vertical delay" (VDELP0/VDELP1/VDELBL)
    uint8_t grp0New, grp0Old;
    uint8_t grp1New, grp1Old;
    bool enablNew, enablOld;

    SDL_Renderer* renderer;
    SDL_Texture* texture;
    std::vector<uint32_t> framebuffer;
    bool frameReady;
};
