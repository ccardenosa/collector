FROM module-srcs:latest AS patcher

FROM fc36-base:latest AS task-master

ARG USE_KERNELS_FILE=false
ENV USE_KERNELS_FILE=$USE_KERNELS_FILE

ARG USE_REMOTE_BUCKET=false
ENV USE_REMOTE_BUCKET=$USE_REMOTE_BUCKET

COPY /kernel-modules/dockerized/scripts/ /scripts/
COPY /kernel-modules/build/apply-blocklist.py /scripts/
COPY /kernel-modules/BLOCKLIST /scripts/
COPY /kernel-modules/dockerized/BLOCKLIST /scripts/dockerized/
COPY /kernel-modules/KERNEL_VERSIONS /KERNEL_VERSIONS
COPY /.openshift-ci/drivers/task-splitter.py /scripts/

COPY --from=patcher /kobuild-tmp/versions-src /kobuild-tmp/versions-src

RUN /scripts/get-build-tasks.sh && /scripts/task-splitter.py

FROM registry.fedoraproject.org/fedora:36

COPY --from=task-master /fc36-tasks /fc36-tasks
COPY --from=task-master /rhel8-tasks /rhel8-tasks
