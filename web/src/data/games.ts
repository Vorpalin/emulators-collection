import fs from "node:fs";
import { ConsoleType, type Game } from "../types/Game";

export function getGames(folder: string): Game[] {
  const result: Game[] = [];

  fs.readdirSync(folder).forEach((file: string) => {
    const path = `${folder}/${file}`;

    if (file.endsWith(".ch8")) {
      result.push({
        name: file.replace(".ch8", ""),
        path,
        console: ConsoleType.Chip8,
      });
    } else if (file.endsWith(".gb")) {
      result.push({
        name: file.replace(".gb", ""),
        path,
        console: ConsoleType.GameBoy,
      });
    } else if (file.endsWith(".a26")) {
      result.push({
        name: file.replace(".a26", ""),
        path,
        console: ConsoleType.Atari2600,
      });
    }
  });

  return result;
}
