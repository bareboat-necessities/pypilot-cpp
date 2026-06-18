#!/usr/bin/env bash
set -euo pipefail

git submodule update --remote --recursive
git submodule status --recursive
