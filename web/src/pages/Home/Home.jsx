import { GameCard } from "../../components/GameCard/GameCard";
import { getGames } from "../../data/games";

function Home() {
  const games = getGames("./games");

  return (
    <main>
      <h1>Games</h1>

      <div className="games-grid">
        {games.map((game) => (
          <GameCard key={game.path} game={game} />
        ))}
      </div>
    </main>
  );
}

export default Home;
