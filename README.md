# Docker Image with Cronet Linux binaries

Without arguments, it applies our custom code to the latest Chromium code. 
With the build argument `CHROMIUM_VERSION` a specific version can be built.

The instructions below assume you have switched to the chromium/src directory.

## Docker Build

```sh
docker build components/cronet/native/docker \
	--build-arg CHROMIUM_VERSION=121.0.6103.0 \
	--tag cronet:121.0.6103.0
```

## Cloud Build

```sh
gcloud builds submit components/cronet/native/docker \
	--config=components/cronet/native/docker/cloudbuild.yaml \ 
	--substitutions=_CHROMIUM_VERSION="121.0.6103.0" \
	--async
```
