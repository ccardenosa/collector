#!/usr/bin/env bash
set -eo pipefail

oc registry login

oc image mirror collector-full quay.io/rhacs-eng/collector:osci-test
echo "COLLECTOR_BUILDER= $COLLECTOR_BUILDER"
