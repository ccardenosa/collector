ARG BASE_REGISTRY=registry.access.redhat.com
ARG BASE_IMAGE=ubi8/ubi-minimal
ARG BASE_TAG=8.6
FROM ${BASE_REGISTRY}/${BASE_IMAGE}:${BASE_TAG} AS extracted_bundle

ARG collector_version=xxx
ARG module_version=xxx
ARG ROOT_DIR=.
ENV ROOT_DIR=$ROOT_DIR

COPY . /go/src/github.com/stackrox/collector
WORKDIR /go/src/github.com/stackrox/collector

RUN mkdir /bundle

RUN cp "${ROOT_DIR}/bundle.tar.gz" /
RUN cp "${ROOT_DIR}/extract-bundle.sh" /bundle/
RUN cp "${ROOT_DIR}"/scripts/* /

RUN echo "${collector_version}" > /COLLECTOR_VERSION
RUN echo "${module_version}" > /MODULE_VERSION

RUN cat /MODULE_VERSION

RUN if [[ "${collector_version}" == "xxx" ]]; then \
     microdnf upgrade -y; \
     microdnf install -y git; \
     git describe --tags --abbrev=10 --dirty > /COLLECTOR_VERSION; \
     echo "collector_version= $collector_version"; \
    fi 

RUN if [[ "${module_version}" == "xxx" ]]; then \
     #module_version="$(cat "${ROOT_DIR}"/../../../kernel-modules/MODULE_VERSION)"; \
     #cat "${ROOT_DIR}"/../../../kernel-modules/MODULE_VERSION > /MODULE_VERSION; \
     cp "${ROOT_DIR}"/../../../kernel-modules/MODULE_VERSION /MODULE_VERSION; \
     echo "module_version= $module_version"; \
    fi 

RUN cat /COLLECTOR_VERSION
RUN cat /MODULE_VERSION

WORKDIR /bundle
RUN ./extract-bundle.sh

FROM ${BASE_REGISTRY}/${BASE_IMAGE}:${BASE_TAG}


ENV COLLECTOR_HOST_ROOT=/host

ARG ROOT_DIR=.
ENV ROOT_DIR=$ROOT_DIR

LABEL name="collector" \
      vendor="StackRox" \
      maintainer="support@stackrox.com" \
      summary="Runtime data collection for the StackRox Kubernetes Security Platform" \
      description="This image supports runtime data collection in the StackRox Kubernetes Security Platform." \
      io.stackrox.collector.module-version="${module_version}" \
      io.stackrox.collector.version="${collector_version}"

WORKDIR /

COPY --from=extracted_bundle /collector-wrapper.sh /
COPY --from=extracted_bundle /bootstrap.sh /
COPY --from=extracted_bundle /bundle/THIRD_PARTY_NOTICES/ /THIRD_PARTY_NOTICES/
COPY --from=extracted_bundle /bundle/kernel-modules/ /kernel-modules/
COPY --from=extracted_bundle /bundle/usr/local/lib/libsinsp-wrapper.so /usr/local/lib/
COPY --from=extracted_bundle /bundle/usr/local/bin/collector /usr/local/bin/
COPY --from=extracted_bundle /COLLECTOR_VERSION /
COPY --from=extracted_bundle /MODULE_VERSION /kernel-modules/MODULE_VERSION.txt

RUN cat /COLLECTOR_VERSION
RUN cat /kernel-modules/MODULE_VERSION.txt

ENV COLLECTOR_VERSION=${collector_version} \
    MODULE_VERSION=${module_version}

COPY final-step.sh /

RUN ./final-step.sh && rm -f final-step.sh


EXPOSE 8080 9090

ENTRYPOINT ["/bootstrap.sh"]

CMD collector-wrapper.sh \
    --collector-config=$COLLECTOR_CONFIG \
    --collection-method=$COLLECTION_METHOD \
    --grpc-server=$GRPC_SERVER
