#!/usr/bin/env bash
#
# Build and run Emulators Collection in Docker, then remove the container when it stops.
#
#   ./scripts/run.sh [dev|prod] [options]
#
# The container is always removed on exit (Ctrl+C, error, normal stop). The image is kept
# so the next start is fast, unless --remove-image is given.

set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONTAINER_NAME="emulators-collection-run"
ENV_FILE="${ROOT_DIR}/web/.env.local"

MODE="dev"
PORT=""
OPEN_BROWSER=0
REMOVE_IMAGE=0

usage() {
  cat <<'EOF'
Usage: scripts/run.sh [dev|prod] [options]

Modes:
  dev                 Vite development server with hot reload (default, port 5173)
  prod                Static site served by nginx (port 8080)

Options:
  -p, --port PORT     Port to publish on the host
  -o, --open          Open the app in the default browser once it is ready
      --remove-image  Also remove the Docker image on exit
  -h, --help          Show this help

The prod mode needs VITE_SUPABASE_URL and VITE_SUPABASE_ANON_KEY, taken from the
environment or from web/.env.local.

Examples:
  scripts/run.sh
  scripts/run.sh --open --port 3000
  scripts/run.sh prod --remove-image
EOF
}

log() { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
die() {
  printf '\033[1;31merror:\033[0m %s\n' "$*" >&2
  exit 1
}

while (($#)); do
  case "$1" in
    dev | prod) MODE="$1" ;;
    -p | --port)
      [[ $# -ge 2 ]] || die "$1 needs a value"
      PORT="$2"
      shift
      ;;
    --port=*) PORT="${1#*=}" ;;
    -o | --open) OPEN_BROWSER=1 ;;
    --remove-image) REMOVE_IMAGE=1 ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      die "unknown argument: $1"
      ;;
  esac
  shift
done

if [[ "$MODE" == "prod" ]]; then
  IMAGE="emulators-collection:prod"
  CONTAINER_PORT=8080
else
  IMAGE="emulator-collection"
  CONTAINER_PORT=5173
fi
PORT="${PORT:-$CONTAINER_PORT}"
[[ "$PORT" =~ ^[0-9]+$ ]] && ((PORT >= 1 && PORT <= 65535)) || die "invalid port: $PORT"
URL="http://localhost:${PORT}"

command -v docker >/dev/null 2>&1 || die "docker is not installed or not in PATH"
docker info >/dev/null 2>&1 || die "the Docker daemon is not running (or you lack permission to use it)"

OPENER_PID=""

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  if [[ -n "$OPENER_PID" ]]; then
    kill "$OPENER_PID" 2>/dev/null || true
  fi
  log "Cleaning up"
  docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
  if ((REMOVE_IMAGE)); then
    docker image rm -f "$IMAGE" >/dev/null 2>&1 || true
  fi
  exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# Read KEY from the environment, then from web/.env.local (quotes and CR stripped).
env_value() {
  local key="$1" value="${!1:-}"
  if [[ -z "$value" && -f "$ENV_FILE" ]]; then
    value="$(grep -E "^${key}=" "$ENV_FILE" | tail -n1 | cut -d= -f2- | tr -d '\r' || true)"
    value="${value%[\"\']}"
    value="${value#[\"\']}"
  fi
  printf '%s' "$value"
}

build_image() {
  if docker buildx version >/dev/null 2>&1; then
    docker buildx build --load "$@"
  else
    DOCKER_BUILDKIT=1 docker build "$@"
  fi
}

wait_then_open() {
  local opener=""
  if command -v xdg-open >/dev/null 2>&1; then
    opener="xdg-open"
  elif command -v open >/dev/null 2>&1; then
    opener="open"
  else
    log "No browser opener found, open ${URL} manually"
    return 0
  fi
  command -v curl >/dev/null 2>&1 || {
    log "curl is missing, open ${URL} manually"
    return 0
  }
  for _ in $(seq 1 120); do
    if curl -fsS -o /dev/null "$URL" 2>/dev/null; then
      "$opener" "$URL" >/dev/null 2>&1 || true
      return 0
    fi
    sleep 1
  done
}

cd "$ROOT_DIR"

# A leftover container from a previous crashed run would block the name and the port.
docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true

if [[ "$MODE" == "prod" ]]; then
  SUPABASE_URL="$(env_value VITE_SUPABASE_URL)"
  SUPABASE_KEY="$(env_value VITE_SUPABASE_ANON_KEY)"
  [[ -n "$SUPABASE_URL" && -n "$SUPABASE_KEY" ]] ||
    die "prod mode needs VITE_SUPABASE_URL and VITE_SUPABASE_ANON_KEY (environment or web/.env.local)"

  log "Building the production image (${IMAGE})"
  build_image --target production \
    --build-arg "VITE_SUPABASE_URL=${SUPABASE_URL}" \
    --build-arg "VITE_SUPABASE_ANON_KEY=${SUPABASE_KEY}" \
    -t "$IMAGE" .
else
  log "Building the development image (${IMAGE})"
  build_image -t "$IMAGE" .
fi

TTY_FLAGS=()
if [[ -t 0 && -t 1 ]]; then
  TTY_FLAGS=(-it)
fi

if ((OPEN_BROWSER)); then
  wait_then_open &
  OPENER_PID=$!
fi

log "Running on ${URL} (Ctrl+C to stop, the container is removed afterwards)"
docker run --rm ${TTY_FLAGS[@]+"${TTY_FLAGS[@]}"} \
  --name "$CONTAINER_NAME" \
  -p "${PORT}:${CONTAINER_PORT}" \
  "$IMAGE"
