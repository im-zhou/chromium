FROM ubuntu:noble AS base

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates \
    tzdata \
    libnss3 \
    libglib2.0

#
# Use the previous stage as a new temporary stage for building libraries
#
FROM base AS build1

# Install Chromium build dependencies.
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    git \
    curl \
    sudo \
    file \
    python3 \
    lsb-release \
    build-essential \
    keyboard-configuration

# Install Chromium's depot_tools.
WORKDIR workspace

RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
    && echo -e "\n# Add Chromium's depot_tools to the PATH." >> ~/.bashrc \
    && echo "export PATH=\"/workspace/depot_tools:$PATH\"" >> ~/.bashrc

ENV PATH /workspace/depot_tools:$PATH

ARG CHROMIUM_VERSION
ARG DEPTH=100

WORKDIR chromium

# Get Chromium code
# Install additional build dependencies
# Checkout specific version if needed
# Process the hooks configured in the DEPS file
RUN fetch --nohooks --no-history chromium \
    && cd src \
    && build/install-build-deps.sh \
    && if [ -n "${CHROMIUM_VERSION}" ] ; then \
    git fetch https://chromium.googlesource.com/chromium/src.git \
    +refs/tags/${CHROMIUM_VERSION}:chromium_${CHROMIUM_VERSION} \
    --depth ${DEPTH} \
    && git checkout tags/${CHROMIUM_VERSION} \
    && gclient sync \
    && gclient sync --with_branch_heads \
    ; fi \
    && gclient runhooks

FROM build1 AS build2

WORKDIR src

ARG DEPTH=100

# Apply weblifeio customization
RUN git remote add weblifeio https://github.com/weblifeio/chromium \
    && git fetch weblifeio --depth ${DEPTH} \
    && git config user.email "<>" \
    && git config user.name "$(whoami)" \
    && git cherry-pick weblifeio/develop ^weblifeio/main

FROM build2 AS build3

# Build Cronet
RUN gn gen out/Cronet --args="is_debug=false icu_use_data_file=false" \
    && ninja -C out/Cronet \
    "cronet_package" \
    "cronet_sample" \
    "extensions/common:url_pattern_api" \
    "extensions/common:url_pattern_api_sample"

#
# Copy libraries to the final image
#
FROM base AS result

COPY --from=build3 /workspace/chromium/src/out/Cronet/cronet/include/* /usr/local/include/cronet/
COPY --from=build3 /workspace/chromium/src/out/Cronet/*.so /usr/local/lib/cronet/
COPY --from=build3 /workspace/chromium/src/out/Cronet/*_sample /usr/local/bin/

RUN ln -s /usr/local/lib/cronet/libcronet.*.so /usr/local/lib/cronet/libcronet.so

ENV LD_LIBRARY_PATH /usr/local/lib/cronet
