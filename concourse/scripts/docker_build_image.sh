#!/bin/bash

set -exo pipefail

DockerFolder="~/plcontainer_src/dockerfiles/$language/"
docker_build() {
	local node=$1
	ssh $node "bash -c \" mkdir -p ~/artifacts_$language\" "

	scp -r datascience/Data*.gppkg $node:~/artifacts_$language
	scp -r plcontainer_src $node:~/
	if [[ $language = "python" ]]; then
		scp -r python/python*.targz $node:~/artifacts_python
		scp -r openssl/openssl*.gz $node:~/artifacts_python
	elif [[ $language = "r" ]]; then
		scp -r r/bin_r_*.tar.gz $node:~/artifacts_r
	else
		echo "Wrong language in pipeline." || exit 1
	fi

	ssh $node "bash -c \" \
	set -eox pipefail; \
	cp ~/artifacts_$language/* $DockerFolder; \
	pushd $DockerFolder; \
	tar -zxvf DataScience*.gppkg; \
	chmod +x *.sh; \
	docker build -f Dockerfile.$language -t pivotaldata/plcontainer_${language}_shared:devel ./ ; \
	popd; \
	docker save pivotaldata/plcontainer_${language}_shared:devel | gzip -c > ~/plcontainer-$language-devel-images.tar.gz; \""
}

docker_build mdw
scp mdw:~/plcontainer-$language-devel-images.tar.gz plcontainer_docker_image/plcontainer-$language-devel-images.tar.gz
