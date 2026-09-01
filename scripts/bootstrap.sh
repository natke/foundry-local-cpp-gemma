#!/usr/bin/env bash
set -euo pipefail

version="2.0.1"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
install_dir="${root}/.foundry-local"

case "$(uname -s)-$(uname -m)" in
  Darwin-arm64)
    asset="foundry-local-osx-arm64.tgz"
    ;;
  Linux-x86_64)
    asset="foundry-local-linux-x64.tgz"
    ;;
  Linux-aarch64|Linux-arm64)
    asset="foundry-local-linux-arm64.tgz"
    ;;
  *)
    echo "Unsupported platform: $(uname -s) $(uname -m)" >&2
    exit 1
    ;;
esac

if [[ -f "${install_dir}/include/foundry_local/foundry_local_cpp.h" ]]; then
  echo "Foundry Local ${version} is already installed in ${install_dir}"
  exit 0
fi

mkdir -p "${install_dir}"
url="https://github.com/microsoft/foundry-local/releases/download/v${version}/${asset}"
echo "Downloading ${url}"
curl --fail --location --silent --show-error "${url}" | tar -xzf - -C "${install_dir}"
echo "Installed Foundry Local ${version} in ${install_dir}"
