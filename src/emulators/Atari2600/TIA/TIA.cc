#include "TIA.hh"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kHblankCycles = 68;

uint32_t convertTIAColor(uint8_t tiaColor)
{
    const uint8_t hue = (tiaColor >> 4) & 0x0F; // bits D4-D7
    const uint8_t lum = (tiaColor >> 1) & 0x07; // bits D1-D3

    constexpr uint8_t kLumaTable[8] = {0x00, 0x40, 0x6C, 0x90, 0xB0, 0xC8, 0xDC, 0xEC};
    const double y = kLumaTable[lum] / 255.0;

    double r = y;
    double g = y;
    double b = y;

    if (hue != 0) {
        const double angleDeg = -(static_cast<double>(hue) - 1.0) * (360.0 / 15.0) - 15.0;
        const double angleRad = angleDeg * M_PI / 180.0;
        const double chromaAmplitude = 0.30;
        const double i = chromaAmplitude * std::cos(angleRad);
        const double q = chromaAmplitude * std::sin(angleRad);

        r = y + 0.956 * i + 0.621 * q;
        g = y - 0.272 * i - 0.647 * q;
        b = y - 1.105 * i + 1.702 * q;
    }

    const auto clamp8 = [](double v) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
    };

    return (0xFFu << 24) | (static_cast<uint32_t>(clamp8(r)) << 16)
           | (static_cast<uint32_t>(clamp8(g)) << 8) | static_cast<uint32_t>(clamp8(b));
}

bool getPlayfieldBit(uint8_t pf0, uint8_t pf1, uint8_t pf2, int index)
{
    if (index < 4) {
        return ((pf0 >> (4 + index)) & 0x01) != 0;
    }
    if (index < 12) {
        const int bit = 7 - (index - 4);
        return ((pf1 >> bit) & 0x01) != 0;
    }
    const int bit = index - 12;
    return ((pf2 >> bit) & 0x01) != 0;
}

// Décodage du champ de mouvement fin des registres HMxx : les 4 bits de
// poids fort forment une valeur signée (-8..+7), les 4 bits de poids
// faible ne sont pas utilisés par le matériel.
int decodeHM(uint8_t value)
{
    int nibble = (value >> 4) & 0x0F;
    if (nibble & 0x08) {
        nibble -= 16;
    }
    return nibble;
}

// Disposition d'un joueur (NUSIZ0/NUSIZ1, bits D0-D2) : nombre de copies,
// décalage de chaque copie supplémentaire, et facteur de taille.
struct SpriteLayout {
    int copies;
    int offsets[2];
    int sizeMultiplier;
};

SpriteLayout getSpriteLayout(uint8_t nusiz)
{
    switch (nusiz & 0x07) {
    case 0x00: return {1, {0, 0}, 1};   // une copie
    case 0x01: return {2, {16, 0}, 1};  // deux copies, rapprochées
    case 0x02: return {2, {32, 0}, 1};  // deux copies, espacées moyen
    case 0x03: return {3, {16, 32}, 1}; // trois copies, rapprochées
    case 0x04: return {2, {64, 0}, 1};  // deux copies, larges
    case 0x05: return {1, {0, 0}, 2};   // une copie, double largeur
    case 0x06: return {3, {32, 64}, 1}; // trois copies, moyen
    case 0x07: return {1, {0, 0}, 4};   // une copie, quadruple largeur
    default: return {1, {0, 0}, 1};
    }
}
}

TIA::TIA()
  : scanline(0)
  , cycle(0)
  , displayLine(0)
  , vsync(false)
  , vblank(false)
  , wsync(false)
  , inputPorts{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
  , grp0New(0)
  , grp0Old(0)
  , grp1New(0)
  , grp1Old(0)
  , enablNew(false)
  , enablOld(false)
  , renderer(nullptr)
  , texture(nullptr)
  , framebuffer(ScreenWidth * ScreenHeight, 0xFF000000)
  , frameReady(false)
{
    this->reset();
}

TIA::~TIA()
{
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
}

void TIA::reset()
{
    for (auto& reg : registers) {
        reg = 0;
    }
    for (auto& c : collisions) {
        c = 0;
    }

    scanline = 0;
    cycle = 0;
    displayLine = 0;
    vsync = false;
    vblank = false;
    wsync = false;
    for (auto& p : inputPorts) {
        p = 0xFF;
    }

    p0 = MovableObject{};
    p1 = MovableObject{};
    m0 = MovableObject{};
    m1 = MovableObject{};
    bl = MovableObject{};

    grp0New = grp0Old = 0;
    grp1New = grp1Old = 0;
    enablNew = enablOld = false;

    frameReady = false;
    std::fill(framebuffer.begin(), framebuffer.end(), 0xFF000000);
}

void TIA::write(uint8_t reg, uint8_t value)
{
    if (reg >= sizeof(registers)) {
        return;
    }
    registers[reg] = value;

    switch (reg) {
    case 0x00: { // VSYNC
        const bool newVsync = (value & 0x02) != 0;
        if (vsync && !newVsync) {
            // Front descendant : fin de la synchro verticale -> nouvelle trame.
            scanline = 0;
            cycle = 0;
            displayLine = 0;
            frameReady = true;
        }
        vsync = newVsync;
        break;
    }
    case 0x01: // VBLANK
        vblank = (value & 0x02) != 0;
        break;
    case 0x02: // WSYNC
        wsync = true;
        break;
    case 0x03: // RSYNC
        cycle = 0;
        break;
    case 0x10: resetPosition(p0); break; // RESP0
    case 0x11: resetPosition(p1); break; // RESP1
    case 0x12: resetPosition(m0); break; // RESM0
    case 0x13: resetPosition(m1); break; // RESM1
    case 0x14: resetPosition(bl); break; // RESBL
    case 0x1B: // GRP0 (registre à double tampon pour VDELP0)
        grp0Old = grp0New;
        grp0New = value;
        break;
    case 0x1C: // GRP1 (registre à double tampon pour VDELP1)
        grp1Old = grp1New;
        grp1New = value;
        break;
    case 0x1F: // ENABL (registre à double tampon pour VDELBL)
        enablOld = enablNew;
        enablNew = (value & 0x02) != 0;
        break;
    case 0x20: p0.hm = value; break; // HMP0
    case 0x21: p1.hm = value; break; // HMP1
    case 0x22: m0.hm = value; break; // HMM0
    case 0x23: m1.hm = value; break; // HMM1
    case 0x24: bl.hm = value; break; // HMBL
    case 0x2A: // HMOVE
        applyHmove();
        break;
    case 0x2B: // HMCLR
        p0.hm = p1.hm = m0.hm = m1.hm = bl.hm = 0;
        registers[0x20] = registers[0x21] = registers[0x22] = registers[0x23] = registers[0x24] = 0;
        break;
    case 0x2C: // CXCLR
        for (auto& c : collisions) {
            c = 0;
        }
        break;
    default:
        // NUSIZ0/1, COLUPx, COLUBK, CTRLPF, REFP0/1, PF0-2, ENAM0/1,
        // VDELP0/1, VDELBL, RESMP0/1, AUDxx : simplement stockés dans
        // `registers[]` et relus directement là où ils sont utilisés.
        break;
    }
}

uint8_t TIA::read(uint8_t reg)
{
    if (reg < 8) {
        // CXM0P, CXM1P, CXP0FB, CXP1FB, CXM0FB, CXM1FB, CXBLPF, CXPPMM
        return collisions[reg];
    }
    if (reg >= 0x08 && reg <= 0x0D) {
        // INPT0..INPT5 : INPT0-INPT3 (paddles) ne sont pas simulées et
        // renvoient l'état neutre par défaut (0xFF) ; INPT4/INPT5
        // (boutons de tir) reflètent ce qui a été poussé par setInputPort.
        return inputPorts[reg - 0x08];
    }
    if (reg < sizeof(registers)) {
        return registers[reg];
    }
    return 0xFF;
}

void TIA::resetPosition(MovableObject& obj)
{
    // Approximation : position = colonne visible correspondant au cycle
    // courant. Le vrai TIA ajoute un petit délai interne (~5 cycles) non
    // modélisé ici.
    int x = cycle - kHblankCycles;
    if (x < 0) {
        x = 0;
    } else if (x >= ScreenWidth) {
        x = ScreenWidth - 1;
    }
    obj.position = x;
}

void TIA::applyHmove()
{
    const auto move = [](MovableObject& obj) {
        int newPos = (obj.position - decodeHM(obj.hm)) % ScreenWidth;
        if (newPos < 0) {
            newPos += ScreenWidth;
        }
        obj.position = newPos;
    };
    move(p0);
    move(p1);
    move(m0);
    move(m1);
    move(bl);
}

bool TIA::playfieldPixel(int x) const
{
    const uint8_t pf0 = registers[0x0D];
    const uint8_t pf1 = registers[0x0E];
    const uint8_t pf2 = registers[0x0F];
    const uint8_t ctrlpf = registers[0x0A];
    const bool reflect = (ctrlpf & 0x01) != 0;

    int pfIndex;
    if (x < ScreenWidth / 2) {
        pfIndex = x / 4;
    } else {
        const int rx = x - ScreenWidth / 2;
        pfIndex = reflect ? (19 - rx / 4) : (rx / 4);
    }
    return getPlayfieldBit(pf0, pf1, pf2, pfIndex);
}

bool TIA::ballPixel(int x) const
{
    const bool delayed = (registers[0x27] & 0x01) != 0; // VDELBL
    const bool enabled = delayed ? enablOld : enablNew;
    if (!enabled) {
        return false;
    }
    const uint8_t ctrlpf = registers[0x0A];
    const int width = 1 << ((ctrlpf >> 4) & 0x03); // 1, 2, 4 ou 8 px
    return x >= bl.position && x < bl.position + width;
}

bool TIA::missilePixel(const MovableObject& obj, uint8_t nusiz, bool enabled, int x) const
{
    if (!enabled) {
        return false;
    }
    const int width = 1 << ((nusiz >> 4) & 0x03); // 1, 2, 4 ou 8 px
    return x >= obj.position && x < obj.position + width;
}

bool TIA::playerPixel(const MovableObject& obj, uint8_t grp, uint8_t nusiz, bool reflect, int x) const
{
    if (grp == 0) {
        return false;
    }
    const SpriteLayout layout = getSpriteLayout(nusiz);
    const int spriteWidth = 8 * layout.sizeMultiplier;

    for (int c = 0; c < layout.copies; ++c) {
        const int offset = (c == 0) ? 0 : layout.offsets[c - 1];
        const int start = (obj.position + offset) % ScreenWidth;

        int rel = x - start;
        if (rel < 0) {
            rel += ScreenWidth; // le sprite peut "enjamber" le bord droit
        }
        if (rel >= 0 && rel < spriteWidth) {
            const int bitIndex = rel / layout.sizeMultiplier;
            const int bit = reflect ? bitIndex : (7 - bitIndex);
            if ((grp >> bit) & 0x01) {
                return true;
            }
        }
    }
    return false;
}

void TIA::updateCollisions(bool pf, bool blPix, bool m0Pix, bool m1Pix, bool p0Pix, bool p1Pix)
{
    if (m0Pix && p1Pix) collisions[0] |= 0x80; // CXM0P  D7 = M0/P1
    if (m0Pix && p0Pix) collisions[0] |= 0x40; // CXM0P  D6 = M0/P0
    if (m1Pix && p0Pix) collisions[1] |= 0x80; // CXM1P  D7 = M1/P0
    if (m1Pix && p1Pix) collisions[1] |= 0x40; // CXM1P  D6 = M1/P1
    if (p0Pix && pf)    collisions[2] |= 0x80; // CXP0FB D7 = P0/PF
    if (p0Pix && blPix) collisions[2] |= 0x40; // CXP0FB D6 = P0/BL
    if (p1Pix && pf)    collisions[3] |= 0x80; // CXP1FB D7 = P1/PF
    if (p1Pix && blPix) collisions[3] |= 0x40; // CXP1FB D6 = P1/BL
    if (m0Pix && pf)    collisions[4] |= 0x80; // CXM0FB D7 = M0/PF
    if (m0Pix && blPix) collisions[4] |= 0x40; // CXM0FB D6 = M0/BL
    if (m1Pix && pf)    collisions[5] |= 0x80; // CXM1FB D7 = M1/PF
    if (m1Pix && blPix) collisions[5] |= 0x40; // CXM1FB D6 = M1/BL
    if (blPix && pf)    collisions[6] |= 0x80; // CXBLPF D7 = BL/PF
    if (p0Pix && p1Pix) collisions[7] |= 0x80; // CXPPMM D7 = P0/P1
    if (m0Pix && m1Pix) collisions[7] |= 0x40; // CXPPMM D6 = M0/M1
}

void TIA::drawPixel(int x, int y)
{
    if (vblank) {
        framebuffer[y * ScreenWidth + x] = 0xFF000000;
        return;
    }

    const uint8_t nusiz0 = registers[0x04];
    const uint8_t nusiz1 = registers[0x05];
    const uint8_t ctrlpf = registers[0x0A];
    const bool reflect0 = (registers[0x0B] & 0x08) != 0; // REFP0 D3
    const bool reflect1 = (registers[0x0C] & 0x08) != 0; // REFP1 D3

    const bool delayP0 = (registers[0x25] & 0x01) != 0; // VDELP0
    const bool delayP1 = (registers[0x26] & 0x01) != 0; // VDELP1
    const uint8_t grp0 = delayP0 ? grp0Old : grp0New;
    const uint8_t grp1 = delayP1 ? grp1Old : grp1New;

    const bool enam0 = (registers[0x1D] & 0x02) != 0;
    const bool enam1 = (registers[0x1E] & 0x02) != 0;

    // RESMP0/1 : verrouille le missile sur la position de son joueur
    // (utilisé par ex. par Combat pour le canon des chars).
    MovableObject m0eff = m0;
    if (registers[0x28] & 0x02) {
        m0eff.position = p0.position;
    }
    MovableObject m1eff = m1;
    if (registers[0x29] & 0x02) {
        m1eff.position = p1.position;
    }

    const bool pf = playfieldPixel(x);
    const bool blPix = ballPixel(x);
    const bool m0Pix = missilePixel(m0eff, nusiz0, enam0, x);
    const bool m1Pix = missilePixel(m1eff, nusiz1, enam1, x);
    const bool p0Pix = playerPixel(p0, grp0, nusiz0, reflect0, x);
    const bool p1Pix = playerPixel(p1, grp1, nusiz1, reflect1, x);

    updateCollisions(pf, blPix, m0Pix, m1Pix, p0Pix, p1Pix);

    const bool priority = (ctrlpf & 0x04) != 0;  // CTRLPF D2 : PF/BL passent devant
    const bool scoreMode = (ctrlpf & 0x02) != 0; // CTRLPF D1 : mode "score"

    uint8_t pfColor = registers[0x08]; // COLUPF
    if (scoreMode) {
        pfColor = (x < ScreenWidth / 2) ? registers[0x06] /* COLUP0 */ : registers[0x07] /* COLUP1 */;
    }

    uint8_t colorReg;
    if (priority) {
        if (pf || blPix) {
            colorReg = pfColor;
        } else if (p0Pix || m0Pix) {
            colorReg = registers[0x06];
        } else if (p1Pix || m1Pix) {
            colorReg = registers[0x07];
        } else {
            colorReg = registers[0x09]; // COLUBK
        }
    } else {
        if (p0Pix || m0Pix) {
            colorReg = registers[0x06];
        } else if (p1Pix || m1Pix) {
            colorReg = registers[0x07];
        } else if (blPix) {
            colorReg = registers[0x08];
        } else if (pf) {
            colorReg = pfColor;
        } else {
            colorReg = registers[0x09];
        }
    }

    framebuffer[y * ScreenWidth + x] = convertTIAColor(colorReg);
}

void TIA::tick()
{
    if (cycle >= kHblankCycles) {
        const int x = cycle - kHblankCycles;
        if (displayLine >= 0 && displayLine < ScreenHeight) {
            drawPixel(x, displayLine);
        }
    }

    ++cycle;
    if (cycle >= 228) {
        cycle = 0;
        wsync = false; // le CPU peut reprendre au début de la nouvelle ligne
        ++scanline;
        if (!vblank && displayLine < ScreenHeight) {
            ++displayLine;
        }
    }

    if (scanline >= 262) {
        // Filet de sécurité si la ROM ne pilote pas VSYNC correctement :
        // on boucle quand même pour ne jamais rester bloqué.
        scanline = 0;
        displayLine = 0;
        frameReady = true;
    }
}

void TIA::setInputPort(uint8_t port, uint8_t value)
{
    if (port < 6) {
        inputPorts[port] = value;
    }
}

void TIA::setRenderer(SDL_Renderer* renderer)
{
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    this->renderer = renderer;
    if (this->renderer != nullptr) {
        SDL_RenderSetLogicalSize(this->renderer, ScreenWidth, ScreenHeight);
        texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, ScreenWidth, ScreenHeight);
    }
}

bool TIA::isFrameReady() const
{
    return frameReady;
}

void TIA::renderFrame()
{
    if (renderer == nullptr || texture == nullptr || !frameReady) {
        return;
    }

    SDL_UpdateTexture(texture, nullptr, framebuffer.data(),
                       ScreenWidth * static_cast<int>(sizeof(uint32_t)));

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    frameReady = false;
}