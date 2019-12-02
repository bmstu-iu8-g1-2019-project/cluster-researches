FROM gossip-base:alpha

ADD ./observer-config.json ./

RUN tree .

EXPOSE 8000

ENTRYPOINT ["./gossip-observer", "./observer-config.json"]