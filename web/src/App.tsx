import { Navigate, Route, Routes } from 'react-router-dom';
import ProtectedRoute from './auth/ProtectedRoute';
import Layout from './components/Layout';
import LoginPage from './pages/LoginPage';
import LibraryPage from './pages/LibraryPage';
import PlayerPage from './pages/PlayerPage';
import ControlsPage from './pages/ControlsPage';

export default function App() {
  return (
    <Routes>
      <Route path="/login" element={<LoginPage />} />

      <Route element={<ProtectedRoute />}>
        <Route element={<Layout />}>
          <Route index element={<LibraryPage />} />
          <Route path="play/:gameId" element={<PlayerPage />} />
          <Route path="controls" element={<ControlsPage />} />
        </Route>
      </Route>

      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  );
}
