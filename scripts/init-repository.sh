#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd -- "$project_dir"
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git init -b main
fi
if [[ $(git rev-parse --show-toplevel) != "$project_dir" ]]; then
  printf 'Refusing to use a repository belonging to a parent directory.\n' >&2
  exit 1
fi
git status --short --branch
printf 'Repository ready; nothing staged or committed.\n'
