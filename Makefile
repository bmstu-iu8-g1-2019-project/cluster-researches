base-image := gossip-base
daemon-image := gossip-daemon
observer-image := gossip-observer
image-tag := alpha

container-port := 8000


build-base:
	docker build -t ${base-image}:${image-tag} -f Dockerfile ./

build-daemon:
	docker build -t ${daemon-image}:${image-tag} ./daemon/

build-observer:
	docker build -t ${observer-image}:${image-tag} ./observer/

run-daemon:
	docker run -d \
	           --name $(name) \
	           --rm \
	           --net macvlan0 \
	           -p $(ip):$(port):${container-port}/udp \
	           ${daemon-image}:${image-tag} \
	           $(ip) \
	           $(port)

run-observer:
	docker run -d \
	           --name $(name) \
    	       --rm \
    	       --net macvlan0 \
    	       -p $(ip):$(port):${container-port}/udp \
    	       ${observer-image}:${image-tag} \
    	       $(ip) \
    	       $(port)
