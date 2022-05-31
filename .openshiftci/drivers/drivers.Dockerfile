FROM fc36-drivers:latest AS fc36-drivers
FROM rhel8-drivers:latest AS rhel8-drivers

FROM registry.fedoraproject.org/fedora:36

COPY --from=fc36-drivers /kernel-modules /kernel-modules
COPY --from=fc36-drivers /FAILURES /FAILURES
COPY --from=rhel8-drivers /kernel-modules /kernel-modules
COPY --from=rhel8-drivers /FAILURES /FAILURES
