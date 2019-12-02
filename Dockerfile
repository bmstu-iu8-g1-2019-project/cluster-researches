FROM ubuntu:latest

RUN apt-get update && \
    apt-get -y install g++ \
                       cmake \
                       make \
                       tree

WORKDIR /src

ADD . .

RUN cmake . && make
RUN tree .

CMD [""]
