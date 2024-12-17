FROM alpine:3.20

ARG github_repo="https://github.com/psp0/opnprojNmap.git"
ARG branch="main"

RUN apk add --update --no-cache \
            ca-certificates \
            libpcap \
            libgcc libstdc++ \
            libssl3 \
 && update-ca-certificates \
 && rm -rf /var/cache/apk/*


RUN apk add --update --no-cache --virtual .build-deps \
        git libpcap-dev lua-dev linux-headers openssl-dev \
        autoconf automake g++ libtool make \
        curl \
    \
 && git clone --depth 1 -b ${branch} ${github_repo} /tmp/nmap \
 && cd /tmp/nmap \
 && ./configure \
        --prefix=/usr \
        --sysconfdir=/etc \
        --mandir=/usr/share/man \
        --infodir=/usr/share/info \
        --without-zenmap \
        --without-nmap-update \
        --with-openssl=/usr/lib \
        --with-liblua=/usr/include \
 && make \
 && make install \
    \
 && apk del .build-deps \
 && rm -rf /var/cache/apk/* \
           /tmp/nmap

ENTRYPOINT ["/usr/bin/nmap"]
