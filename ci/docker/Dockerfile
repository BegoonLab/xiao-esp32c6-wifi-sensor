ARG IDF_VERSION="v5.3.1"
ARG IDF_INSTALL_TARGETS="esp32c6"
ARG DEBIAN_FRONTEND=noninteractive

FROM espressif/idf:${IDF_VERSION} AS esp-idf

ARG ESP_MATTER_CLONE_URL=https://github.com/espressif/esp-matter.git
ARG ESP_MATTER_CHECKOUT_REF=release/v1.4

RUN : \
  && apt-get update \
  && DEBIAN_FRONTEND=${DEBIAN_FRONTEND} apt-get install -fy --no-install-recommends \
    libgirepository1.0-dev \
    libssl-dev \
    pkg-config \
    python3 \
    python3-pip \
  && apt-get clean \
  && apt-get autoremove -y \
  && rm -rf /var/lib/apt/lists/* \
  && : # last line

WORKDIR /opt/esp
ENV ESP_MATTER_PATH=/opt/esp/esp-matter

RUN set -x \
    && mkdir -p $ESP_MATTER_PATH \
    && cd $ESP_MATTER_PATH \
    && git init \
    && git remote add origin $ESP_MATTER_CLONE_URL \
    && git fetch origin --depth=1 ${ESP_MATTER_CHECKOUT_REF} \
    && git checkout FETCH_HEAD \
    && git submodule update --init --depth 1 \
    && cd ./connectedhomeip/connectedhomeip \
    && ./scripts/checkout_submodules.py --platform esp32 linux --shallow \
    && cd ../.. \
    && ./install.sh \
    && : # last line

RUN set -x \
    && git config --global --add safe.directory /opt/esp/project \
    && git config --global --add safe.directory /opt/esp/idf/components/openthread/openthread \
    && mkdir -p /.cache/Espressif/ComponentManager \
    && chmod -R 755 /.cache/Espressif/ComponentManager \
    && : # last line

COPY entrypoint.sh /opt/esp/entrypoint.sh
ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD [ "/bin/bash" ]

WORKDIR /opt/esp/esp-matter