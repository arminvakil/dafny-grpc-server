FROM ubuntu:20.04

# Install required dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y \
    wget \
    curl \
    git \
    unzip \
    build-essential \
    openjdk-11-jdk \
    python3 \
    gnupg \
    apt-transport-https \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg \
    && mv bazel.gpg /etc/apt/trusted.gpg.d/ \
    && echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y bazel-4.0.0

RUN wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb \
    && dpkg -i packages-microsoft-prod.deb \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y apt-transport-https dotnet-sdk-6.0 aspnetcore-runtime-6.0 dotnet-runtime-6.0 \
    # && wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh \
    # && chmod +x dotnet-install.sh \
    # && ./dotnet-install.sh --version 8.0.111 \
    # && ln -s /root/.dotnet/dotnet /usr/local/bin/dotnet \
    && rm -rf /var/lib/apt/lists/*

# ENV PATH="$PATH:/root/.dotnet"

# Set working directory
WORKDIR /home/

# Clone and build Dafny
RUN git clone https://github.com/dafny-lang/dafny.git dafny \
    && cd dafny \
    && git checkout v4.9.0 \
    && dotnet build Source/Dafny.sln \
    && make z3-ubuntu

# Clone and build dafny-grpc-server
RUN git clone https://github.com/arminvakil/dafny-grpc-server.git dafny-grpc-server \
    && cd dafny-grpc-server \
    && bazel-4.0.0 build --cxxopt="-g" --cxxopt="--std=c++17" //src:server

# Set environment variables
ENV DAFNY_EXE_PATH=/home/dafny/Binaries/Dafny
# RUN echo $PATH

# Default command
# CMD ["/home/dafny-grpc-server/bazel-bin/src/server", "-d", "/home/dafny/Binaries/Dafny", "-j", "1", "-v"]
