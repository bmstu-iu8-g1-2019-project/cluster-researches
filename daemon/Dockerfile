FROM gossip-base:alpha

ADD ./daemon-config.json ./

RUN tree .

EXPOSE 8000

ENTRYPOINT ["./gossip-daemon", "./daemon-config.json"]