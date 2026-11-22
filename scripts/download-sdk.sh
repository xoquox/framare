#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
download_dir="$project_dir/.local/downloads"
installer_name=SailfishSDK-3.13.5-linux64-online.run
installer_url="https://releases.sailfishos.org/sdk/installers/3.13.5/$installer_name"
# Published on https://docs.sailfishos.org/Tools/Sailfish_SDK/
# MD5 and size detect download corruption; authenticity relies on HTTPS.
expected_md5=20fbb809a4ad27b71444a2081d56ca84
expected_size=30408560
# SHA256 recorded after the initial HTTPS download matched published MD5/size.
expected_sha256=1b5703da664a05743ffa93e68cff874f17f21076358adeb85145bd62bf0ddb86
mkdir -p -- "$download_dir"
if [[ -f "$download_dir/$installer_name" ]]; then
  existing_sha256=$(sha256sum "$download_dir/$installer_name")
  if [[ ${existing_sha256%% *} == "$expected_sha256" ]]; then
    printf 'SDK installer already downloaded and verified: %s\n' "$download_dir/$installer_name"
    exit 0
  fi
fi
temporary=$(mktemp "$download_dir/.sdk-download.XXXXXX")
trap 'rm -f -- "$temporary"' EXIT
curl --proto '=https' --tlsv1.2 --fail --location --show-error \
  --retry 2 --connect-timeout 15 --max-time 300 \
  --output "$temporary" "$installer_url"
python3 - "$temporary" "$expected_md5" "$expected_size" "$expected_sha256" <<'PY'
import hashlib
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
if len(data) != int(sys.argv[3]):
    sys.exit("SDK download has an unexpected size; file rejected")
if hashlib.md5(data, usedforsecurity=False).hexdigest() != sys.argv[2]:
    sys.exit("SDK download does not match the published checksum; file rejected")
if hashlib.sha256(data).hexdigest() != sys.argv[4]:
    sys.exit("SDK download does not match the recorded SHA256; file rejected")
print("SDK SHA256:", hashlib.sha256(data).hexdigest())
PY
chmod 0755 "$temporary"
mv -- "$temporary" "$download_dir/$installer_name"
printf 'Downloaded and checked: %s\n' "$download_dir/$installer_name"
