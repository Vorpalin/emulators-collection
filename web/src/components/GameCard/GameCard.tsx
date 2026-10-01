import type { Game } from "../types/Game";


export function GameCard({ game }: { game: Game }) {
  const handleClick = () => {
    console.log(game.path);
  };

  return (
    <div className="game-card">
      <h3>{game.name}</h3>

      <button onClick={handleClick}>
        Launch {game.name} on {game.console}
      </button>
    </div>
  );
}
