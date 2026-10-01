import AuthForm from "../../components/AuthForm/AuthForm";

export default function Register() {
  return (
    <main>
      <h1>Créer un compte</h1>

      <AuthForm mode="register" />

      <a href="/login">
        Déjà un compte ? Se connecter
      </a>
    </main>
  );
}
