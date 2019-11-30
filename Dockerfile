FROM ubuntu:latest

RUN apt-get update && \
    apt-get -y install g++ \
                       cmake \
                       make \
                       tree

ADD ./cmake-build-debug/daemon /src/
ADD ./configs /src/

WORKDIR /src

VOLUME /src/configs

RUN ls ./configs

RUN tree .

EXPOSE 8000/udp

ENTRYPOINT ["./daemon"]
