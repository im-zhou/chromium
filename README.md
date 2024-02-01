# Docker Image with Cronet Linux binaries

Without arguments, it applies our custom code to the latest Chromium code. 
With the build argument `CHROMIUM_VERSION` a specific version can be built.

## Clone single branch

```sh
git clone -b build/cronet --single-branch git@github.com:weblifeio/chromium.git cronet
```

## Docker Build

```sh
docker build . \
	--file Dockerfile \
	--build-arg CHROMIUM_VERSION="121.0.6167.85" \
	--tag cronet:"121.0.6167.85"
```

## Cloud Build

```sh
gcloud builds submit . \
	--config=cloudbuild.yaml \
	--substitutions=_CHROMIUM_VERSION="121.0.6167.85" \
	--async
```
