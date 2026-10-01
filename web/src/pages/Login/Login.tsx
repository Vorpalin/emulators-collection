import AuthForm from "../../components/AuthForm/AuthForm";

export default function Login() {
  return (
    <main>
      <h1>Connexion</h1>

      <AuthForm mode="login" />

      <a href="/register">
        Create an account
      </a>
    </main>
  );
}
