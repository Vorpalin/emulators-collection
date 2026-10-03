#!/usr/bin/env bash

set -euo pipefail

files=()

for file in "$@"; do
    case "$file" in
        *.cjs|*.mjs|*.js|*.jsx|*.ts|*.tsx|*.json|*.jsonc|*.css|*.scss|*.html|*.md|*.yaml|*.yml)
            files+=("$file")
            ;;
    esac
done

if [ "${#files[@]}" -eq 0 ]; then
    exit 0
fi

exec web/node_modules/.bin/prettier --write "${files[@]}"
