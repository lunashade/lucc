FROM ubuntu:18.04
RUN apt update && \
    apt install -y \
    gcc-8-riscv64-linux-gnu git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev wget && \
    apt autoclean && apt autoremove && rm -rf /var/lib/apt/lists*
RUN wget https://download.qemu.org/qemu-5.0.0.tar.xz && \
    tar xvf qemu-5.0.0.tar.xz && rm qemu-5.0.0.tar.xz
WORKDIR qemu-5.0.0
RUN ./configure && make -j && make install
