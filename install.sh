#!/usr/bin/env bash
# Ultimate IAH — installateur automatique Arch Linux / Omarchy.
# Usage :
#   curl -fsSL https://raw.githubusercontent.com/chimprookie-group/ULTIMATE-IAH/main/install.sh | bash
#   curl -fsSL ... | bash -s -- --with-artcraft
#   ./install.sh --check
#   ./install.sh --dry-run
set -Eeuo pipefail

REPO="chimprookie-group/ULTIMATE-IAH"
BASE="https://github.com/${REPO}"
RAW="https://raw.githubusercontent.com/${REPO}/main"
VERSION="1.25.12"
PKG="ultimate-iah-${VERSION}-1-x86_64.pkg.tar.zst"
ASSET="${BASE}/releases/latest/download/${PKG}"
CHECKSUM="${ASSET}.sha256"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}/Crasy/deepcrasy-h"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}/Crasy/deepcrasy-h"
MODE="install"
WITH_ARTCRAFT="false"

for arg in "$@"; do
  case "$arg" in
    --check) MODE="check" ;;
    --dry-run) MODE="dry-run" ;;
    --with-artcraft) WITH_ARTCRAFT="true" ;;
    --help|-h)
      sed -n '1,20p' "$0" 2>/dev/null || true
      echo "Options: --check --dry-run --with-artcraft --help"
      exit 0
      ;;
    *) echo "Option inconnue: $arg" >&2; exit 2 ;;
  esac
done

info() { printf '\033[1;36m[Ultimate IAH]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[Ultimate IAH]\033[0m %s\n' "$*" >&2; }
die() { printf '\033[1;31m[Ultimate IAH]\033[0m %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

[[ "$(uname -s)" == "Linux" ]] || die "Linux requis."
have pacman || die "Ce paquet cible Arch Linux / Omarchy (pacman absent)."
have curl || die "curl est requis."
have pkexec || die "polkit/pkexec est requis pour installer le paquet."

DEPS=(qt6-base qt6-declarative qt6-webengine qt6-webchannel qt6-wayland qt6-svg hicolor-icon-theme xdg-desktop-portal xdg-utils polkit nodejs npm python python-yaml git github-cli)
BUILD_DEPS=(gcc make)

missing=()
for p in "${DEPS[@]}"; do
  pacman -Q "$p" >/dev/null 2>&1 || missing+=("$p")
done
missing_build=()
for p in "${BUILD_DEPS[@]}"; do
  pacman -Q "$p" >/dev/null 2>&1 || missing_build+=("$p")
done

if [[ "${#missing[@]}" -gt 0 ]]; then
  info "Paquets manquants: ${missing[*]}"
else
  info "Dépendances système présentes."
fi

if [[ "$MODE" == "check" ]]; then
  info "Version cible: $VERSION"
  info "Paquet: $ASSET"
  info "Config: $CONFIG"
  info "Données: $DATA"
  [[ "${#missing[@]}" -eq 0 ]] || { warn "Installer manquera: ${missing[*]}"; exit 1; }
  info "Contrôle terminé."
  exit 0
fi

if [[ "$MODE" == "dry-run" ]]; then
  echo "Commande d'installation: pkexec pacman -U /tmp/$PKG"
  echo "Bootstrap: ultimate-iah bootstrap"
  echo "ArtCraft: $WITH_ARTCRAFT"
  exit 0
fi

# ---------------------------------------------------------------------------
# Dépendances
# ---------------------------------------------------------------------------
if [[ "${#missing[@]}" -gt 0 || "${#missing_build[@]}" -gt 0 ]]; then
  to_install=("${missing[@]}" "${missing_build[@]}")
  info "Installation des dépendances: ${to_install[*]}"
  pkexec pacman -S --needed --noconfirm "${to_install[@]}"
fi

TMP="$(mktemp -d /tmp/ultimate-iah-install.XXXXXX)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Paquet officiel, avec SHA256 obligatoire
# ---------------------------------------------------------------------------
install_package() {
  info "Téléchargement du paquet $VERSION…"
  if ! curl -fL --retry 3 --connect-timeout 20 -o "$TMP/$PKG" "$ASSET"; then
    warn "Paquet indisponible; construction depuis les sources."
    build_from_source
    return
  fi
  if ! curl -fL --retry 3 --connect-timeout 20 -o "$TMP/$PKG.sha256" "$CHECKSUM"; then
    die "Checksum SHA256 indisponible: installation refusée."
  fi
  expected="$(awk '{print $1}' "$TMP/$PKG.sha256")"
  actual="$(sha256sum "$TMP/$PKG" | awk '{print $1}')"
  [[ -n "$expected" && "$expected" == "$actual" ]] || die "Checksum SHA256 invalide."
  info "Checksum vérifié."
  pkexec pacman -U --noconfirm "$TMP/$PKG"
}

build_from_source() {
  info "Clonage des sources…"
  have git || die "git requis pour la compilation."
  git clone --depth 1 "$BASE.git" "$TMP/src"
  cd "$TMP/src"
  [[ -x ./bin/build ]] || die "bin/build absent."
  [[ -x ./bin/test ]] || die "bin/test absent."
  info "Compilation…"
  ./bin/build
  info "Tests…"
  ./bin/test
  info "Construction du paquet…"
  cd pkgbuild
  makepkg -f
  pkgfile="$(find . -maxdepth 1 -name 'ultimate-iah-*.pkg.tar.zst' -print -quit)"
  [[ -n "$pkgfile" ]] || die "Paquet introuvable après compilation."
  pkexec pacman -U --noconfirm "$pkgfile"
  cd - >/dev/null
}

install_package

# ---------------------------------------------------------------------------
# Bootstrap du harness
# ---------------------------------------------------------------------------
have ultimate-iah || die "ultimate-iah absent après installation."
info "Initialisation des skills, de la configuration et de 4Crasy…"
ultimate-iah bootstrap

info "Diagnostic du harness…"
if ! ultimate-iah doctor --json; then
  warn "Le diagnostic signale un composant manquant; l'application reste installable."
fi

# ---------------------------------------------------------------------------
# ArtCraft optionnel
# ---------------------------------------------------------------------------
if [[ "$WITH_ARTCRAFT" == "true" ]]; then
  info "Installation optionnelle de la suite ArtCraft…"
  have node || die "Node.js requis pour ArtCraft."
  git clone --depth 1 https://github.com/lznnnnn1998/dsh-artcraft "$TMP/artcraft"
  node "$TMP/artcraft/scripts/install-craft.mjs" --dir "$DATA/artcraft/bin"
fi

# ---------------------------------------------------------------------------
# Vérification finale
# ---------------------------------------------------------------------------
info "Vérification finale…"
ultimate-iah --version
[[ -f "$CONFIG/4crasy/token" ]] || die "Jeton 4Crasy absent."
[[ "$(stat -c '%a' "$CONFIG/4crasy/token")" == "600" ]] || die "Permissions du jeton 4Crasy invalides."
info "Installation terminée. Lancez Ultimate IAH depuis le menu Applications."
