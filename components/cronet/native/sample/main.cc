// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cassert>
#include <fstream>
#include <iostream>

#include "cronet_c.h"
#include "sample_executor.h"
#include "sample_url_request_callback.h"

const char* getCmdOption(int argc,
                         char* argv[],
                         const std::string& option,
                         const std::string& dflt = "") {
  char** begin = argv;
  char** end = argv + argc;
  char** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end) {
    return *itr;
  }
  return dflt.c_str();
}

static std::vector<char> ReadAllBytes(const char* filename) {
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  assert(pos > 0);

  std::vector<char> result(pos);

  ifs.seekg(0, std::ios::beg);
  ifs.read(&result[0], pos);

  return result;
}

Cronet_EnginePtr ConfigureClientCertificate(Cronet_EnginePtr cronet_engine,
                                            const char* host_port_pair,
                                            const char* client_cert_file,
                                            const char* private_key_file) {
  if (strlen(host_port_pair) == 0 || strlen(client_cert_file) == 0 ||
      strlen(private_key_file) == 0) {
    return cronet_engine;
  }
  std::vector<char> client_cert_data = ReadAllBytes(client_cert_file);
  std::vector<char> private_key_data = ReadAllBytes(private_key_file);

  assert(client_cert_data.size() > 0);
  assert(private_key_data.size() > 0);

  Cronet_BufferPtr client_cert_buffer = Cronet_Buffer_Create();
  Cronet_Buffer_InitWithDataAndCallback(client_cert_buffer,
                                        client_cert_data.data(),
                                        client_cert_data.size(), nullptr);

  Cronet_BufferPtr private_key_buffer = Cronet_Buffer_Create();
  Cronet_Buffer_InitWithDataAndCallback(private_key_buffer,
                                        private_key_data.data(),
                                        private_key_data.size(), nullptr);

  Cronet_Engine_SetClientCertificate(cronet_engine, host_port_pair,
                                     client_cert_buffer, private_key_buffer);
  Cronet_Buffer_Destroy(client_cert_buffer);
  Cronet_Buffer_Destroy(private_key_buffer);

  return cronet_engine;
}

Cronet_EnginePtr CreateCronetEngine(const std::string& proxy) {
  Cronet_EnginePtr cronet_engine = Cronet_Engine_Create();
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_user_agent_set(engine_params, "CronetSample/1");
  Cronet_EngineParams_enable_quic_set(engine_params, true);
  Cronet_EngineParams_proxy_server_set(engine_params, proxy.c_str());

  Cronet_Engine_StartWithParams(cronet_engine, engine_params);
  Cronet_EngineParams_Destroy(engine_params);
  return cronet_engine;
}

void PerformRequest(Cronet_EnginePtr cronet_engine,
                    const std::string& url,
                    Cronet_ExecutorPtr executor) {
  SampleUrlRequestCallback url_request_callback;
  Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
  Cronet_UrlRequestParamsPtr request_params = Cronet_UrlRequestParams_Create();
  Cronet_UrlRequestParams_http_method_set(request_params, "GET");

  Cronet_UrlRequest_InitWithParams(
      request, cronet_engine, url.c_str(), request_params,
      url_request_callback.GetUrlRequestCallback(), executor);
  Cronet_UrlRequestParams_Destroy(request_params);

  Cronet_UrlRequest_Start(request);
  url_request_callback.WaitForDone();
  Cronet_UrlRequest_Destroy(request);

  std::cout << "Response Data:" << std::endl
            << url_request_callback.response_as_string() << std::endl;
}

// Download a resource from the Internet. Optional argument must specify
// a valid URL.
int main(int argc, char* argv[]) {
  std::cout << "Hello from Cronet!\n";
  std::string proxy(getCmdOption(argc, argv, "--proxy"));
  if (!proxy.empty()) {
    std::cout << "Proxy: " << proxy << std::endl;
  }
  Cronet_EnginePtr cronet_engine = CreateCronetEngine(proxy);
  ConfigureClientCertificate(cronet_engine, getCmdOption(argc, argv, "--host"),
                             getCmdOption(argc, argv, "--cert"),
                             getCmdOption(argc, argv, "--key"));
  std::cout << "Cronet version: "
            << Cronet_Engine_GetVersionString(cronet_engine) << std::endl;

  std::string url(getCmdOption(argc, argv, "--url", "https://www.example.com"));
  std::cout << "URL: " << url << std::endl;
  SampleExecutor executor;
  PerformRequest(cronet_engine, url, executor.GetExecutor());

  Cronet_Engine_Shutdown(cronet_engine);
  Cronet_Engine_Destroy(cronet_engine);
  return 0;
}
