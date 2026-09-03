#!/usr/bin/env bash
set -euo pipefail

version="2.0.1"
ort_genai_version="0.15.2"
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

mkdir -p "${install_dir}"
if [[ ! -f "${install_dir}/include/foundry_local/foundry_local_cpp.h" ]]; then
  url="https://github.com/microsoft/foundry-local/releases/download/v${version}/${asset}"
  echo "Downloading ${url}"
  curl --fail --location --silent --show-error "${url}" | tar -xzf - -C "${install_dir}"
  echo "Installed Foundry Local ${version} in ${install_dir}"
else
  echo "Foundry Local ${version} is already installed in ${install_dir}"
fi

mkdir -p "${install_dir}/include"
for header in ort_genai.h ort_genai_c.h; do
  if [[ ! -f "${install_dir}/include/${header}" ]]; then
    url="https://raw.githubusercontent.com/microsoft/onnxruntime-genai/v${ort_genai_version}/src/${header}"
    echo "Downloading ${url}"
    curl --fail --location --silent --show-error \
      --output "${install_dir}/include/${header}" "${url}"
  fi
done
