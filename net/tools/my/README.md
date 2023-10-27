# My

Simple command line utility that loads a web page and prints it to stdout. If URL starts from `ws://` or `wss://`, then
the program establishes a websocket connection, reads frames for 30 seconds and closes the connection at the end.

The code was written for educational purposes to demonstrate the basic use of the Chromium networking stack.

## Usage

```sh
./my_app <url> [<proxy>]
    url   - Web page URL to open (required)
    proxy - Proxy server to forward the requests to destination (optional)
```

## Checking out the code

Follow all the
[Get the Code](https://www.chromium.org/developers/how-tos/get-the-code)
instructions for your target platform up to and including running hooks.

## Building for development and debugging

First, `gn` is used to create ninja files targeting the intended platform, then
`ninja` executes the ninja files to run the build.

```shell
gn gen out/My
```

### Running the ninja files

Now, use the generated ninja files to execute the build against the
`my_app` build target:

```shell
$ ninja -C out/My my_app
```
