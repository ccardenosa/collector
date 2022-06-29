FROM fc36-base:latest AS patcher

ARG BRANCH=mauro-OSCI-driver-builds
ARG LEGACY_PROBES=false
ENV CHECKOUT_BEFORE_PATCHING=true
ENV DOCKERIZED=1

# This directory goes separately to prevent it from being modified/deleted when switching branches
COPY /kernel-modules/dockerized/scripts /scripts
COPY /kernel-modules/build/prepare-src /scripts/prepare-src.sh
COPY /kernel-modules/build/build-kos /scripts/
COPY /kernel-modules/build/build-wrapper.sh /scripts/compile.sh

COPY / /collector

RUN git -C /collector remote -v && \
	git -C /collector fetch --all && \
	git -C /collector show-ref

RUN /scripts/patch-files.sh $BRANCH $LEGACY_PROBES

FROM registry.fedoraproject.org/fedora:36

COPY --from=patcher /kobuild-tmp/versions-src /kobuild-tmp/versions-src
