FROM quay.io/centos/centos:stream8 AS rhel-8-base

COPY gcloud-sdk-install.sh .

RUN dnf -y update && \
	dnf -y install \
		make \
		cmake \
		gcc-c++ \
		llvm \
		clang \
		elfutils-libelf \
		elfutils-libelf-devel \
		kmod && \
		./gcloud-sdk-install.sh
