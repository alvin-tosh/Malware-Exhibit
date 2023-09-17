FROM ronaldsonbellande/sph_tools

ARG VERSION_GIT_BRANCH=master
ARG VERSION_GIT_COMMIT=HEAD

LABEL maintainer=ronaldsonbellande@gmail.com
LABEL github_branchtag=${VERSION_GIT_BRANCH}
LABEL github_commit=${VERSION_GIT_COMMIT}

# Ubuntu setup
RUN apt-get update -y && apt-get upgrade -y

# Set the working directory in the container
WORKDIR ./Security_Hacking_Scripts

# Copy the entire host directory to the container's working directory
COPY . .

CMD tail -f /dev/null
