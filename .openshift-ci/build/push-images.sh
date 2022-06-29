#!/usr/bin/env bash
set -eo pipefail

oc registry login

echo "COLLECTOR_BUILDER= $COLLECTOR_BUILDER"
