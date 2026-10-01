export enum ConsoleType {
  Chip8 = "Chip8",
  Atari2600 = "Atari2600",
  GameBoy = "GameBoy",
}

export interface Game {
  name: string;
  path: string;
  console: ConsoleType;
}
