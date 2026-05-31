FROM archlinux:latest

ADD https://archive.archlinux.org/packages/w/wine/wine-10.1-1-x86_64.pkg.tar.zst /tmp/
ADD https://archive.archlinux.org/packages/w/wine/wine-10.1-1-x86_64.pkg.tar.zst.sig /tmp/

RUN pacman-key --init && pacman-key --populate archlinux

# Enable multilib
# RUN sed -i '/\[multilib\]/,/Include/s/^#//' /etc/pacman.conf

# Add multilib
RUN echo -e "\n[multilib]\nInclude = /etc/pacman.d/mirrorlist" >> /etc/pacman.conf

RUN pacman -Syu --noconfirm

RUN pacman -U --noconfirm /tmp/wine-10.1-1-x86_64.pkg.tar.zst

RUN pacman -S --noconfirm --needed \
    binutils \
    clang mold ccache \
    lib32-glibc

RUN rm -rf /var/cache/pacman/pkg/* /tmp/*

WORKDIR /build

CMD ["bash"]
