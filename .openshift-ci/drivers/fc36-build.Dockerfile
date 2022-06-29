FROM module-srcs:latest AS patcher
FROM driver-tasks:latest AS tasks

FROM fc36-base:latest AS builder

ENV DOCKERIZED=1
ENV OSCI_RUN=1

# This directory goes separately to prevent it from being modified/deleted when switching branches
COPY /kernel-modules/dockerized/scripts /scripts
COPY /kernel-modules/build/prepare-src /scripts/prepare-src.sh
COPY /kernel-modules/build/build-kos /scripts/
COPY /kernel-modules/build/build-wrapper.sh /scripts/compile.sh

COPY --from=patcher /kobuild-tmp/versions-src /kobuild-tmp/versions-src
COPY --from=tasks /fc36-tasks /fc36-tasks

# For the time being, we will only build 5 drivers for testing
# RUN /scripts/compile.sh < /fc36-tasks
RUN head -n5 /fc36-tasks > /aux-tasks && \
    /scripts/compile.sh < /aux-tasks

FROM registry.fedoraproject.org/fedora:36

COPY --from=builder /kernel-modules /kernel-modules
COPY --from=builder /FAILURES /FAILURES
