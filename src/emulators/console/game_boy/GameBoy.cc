#include "emulators/console/game_boy/GameBoy.hh"

const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;

GameBoy::GameBoy() : bus() {}

GameBoy::~GameBoy() {
  if (texture) {
    SDL_DestroyTexture(texture);
  }
}

void GameBoy::loadProgram(const std::string& filename) {
  reset();
  bus.loadROM(const_cast<std::string&>(filename));
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);
}

void GameBoy::reset() {
  bus.reset();
  isRunning = true;
}

void GameBoy::handleInput() {
  SDL_Event event;
  GameBoyController& joypad = bus.getJoypad();
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      isRunning = false;
    } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      bool pressed = (event.type == SDL_KEYDOWN);
      switch (event.key.keysym.sym) {
        case SDLK_RIGHT:
          joypad.setButton(GameBoyController::Right, pressed);
          break;
        case SDLK_LEFT:
          joypad.setButton(GameBoyController::Left, pressed);
          break;
        case SDLK_ESCAPE:
          isRunning = false;
          break;
        case SDLK_UP:
          joypad.setButton(GameBoyController::Up, pressed);
          break;
        case SDLK_DOWN:
          joypad.setButton(GameBoyController::Down, pressed);
          break;
        case SDLK_a:
          joypad.setButton(GameBoyController::A, pressed);
          break;
        case SDLK_b:
          joypad.setButton(GameBoyController::B, pressed);
          break;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
          joypad.setButton(GameBoyController::Select, pressed);
          break;
        case SDLK_RETURN:
          joypad.setButton(GameBoyController::Start, pressed);
          break;
        default:
          break;
      }
    }
  }
}

void GameBoy::renderFrame() {
  static const uint32_t palette[4] = {
      0xFFFFFFFFu,  // blanc
      0xFFAAAAAAu,  // gris clair
      0xFF555555u,  // gris foncé
      0xFF000000u,  // noir
  };

  const auto& framebuffer = bus.getFramebuffer();  // prolonge la durée de vie
  const uint8_t* shades = framebuffer.data();

  static uint32_t pixels[GB_WIDTH * GB_HEIGHT];
  for (int i = 0; i < GB_WIDTH * GB_HEIGHT; ++i) {
    pixels[i] = palette[shades[i] & 0x03];
  }

  SDL_UpdateTexture(texture, nullptr, pixels, GB_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

int GameBoy::run() {
  while (isRunning) {
    handleInput();
    bus.tick();
    if (bus.consumeFrameReady()) {
      renderFrame();
    }
  }
  return 0;
}
