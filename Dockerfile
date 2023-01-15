FROM ghcr.io/ronaldsonbellande/sph_tools:master

ARG VERSION_GIT_BRANCH=master
ARG VERSION_GIT_COMMIT=HEAD

LABEL maintainer=ronaldsonbellande@gmail.com
LABEL github_branchtag=${VERSION_GIT_BRANCH}
LABEL github_commit=${VERSION_GIT_COMMIT}

CMD [ "bash" ]
