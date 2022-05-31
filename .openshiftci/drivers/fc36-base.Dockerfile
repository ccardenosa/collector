FROM registry.fedoraproject.org/fedora:36 AS builder

COPY gcloud-sdk-install.sh .

RUN dnf -y install \
	make \
	cmake \
	gcc-c++ \
	llvm \
	clang \
	patch \
	elfutils-libelf \
	elfutils-libelf-devel \
	git \
	python3 \
	kmod \
	libxcrypt-compat.x86_64 && \
	./gcloud-sdk-install.sh
