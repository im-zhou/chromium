// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// HTTP server example. This server serves both static and dynamic content.
// It opens two ports: plain HTTP on port 8000 and HTTP on port 8443.
// It implements the following endpoints:
//    /api/stats - respond with free-formatted stats on current connections
//    /api/f2/:id - wildcard example, respond with JSON string {"result": "URI"}
//    any other URI serves static files from s_root_dir
//
// To enable SSL/TLS (using self-signed certificates in PEM files),
//    1. make SSL=OPENSSL or make SSL=MBEDTLS
//    2. curl -k https://127.0.0.1:8443

#include <signal.h>
#include <unistd.h>
#include <cstring>
#include "base/at_exit.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/message_loop/message_pump_type.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "mongoose.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/load_flags.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_info.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

static const char* s_http_addr = "http://0.0.0.0:8000";  // HTTP port
#define BUF_SZ 100 * 1024

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}
class MyDelegate : public net::URLRequest::Delegate {
 public:
  explicit MyDelegate(base::OnceClosure closure, struct mg_connection* c)
      : quit_closure_(std::move(closure)),
        buf_(base::MakeRefCounted<net::IOBuffer>(BUF_SZ)),
        connection_(std::move(c)) {}

  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override {
    std::cerr << "redirect to " << redirect_info.new_url << std::endl;
    *defer_redirect = true;
    std::cerr << "resp started from redirect" << std::endl;
    // Get the header and send
    size_t iter = 0;
    std::string name;
    std::string value;
    std::vector<std::string> response_headers;
    response_headers.push_back(request->response_headers()->GetStatusLine() +
                               "\r\n");
    while (request->response_headers()->EnumerateHeaderLines(&iter, &name,
                                                             &value)) {
      // if(name.compare("Content-Encoding") == 0) continue;
      std::string h = name + ": " + value + "\r\n";
      response_headers.push_back(h);
    }
    for (std::string element : response_headers) {
      mg_send(connection_, &element[0], element.size());
      std::cerr << element << std::endl;
    }
    mg_send(connection_, "\r\n", 2);
    // auto n = request->Read(buf_.get(), BUF_SZ);
    // std::cerr << "resp read " << n << std::endl;

    // if (n == net::ERR_IO_PENDING) {
    //   return;
    // }
    OnReadCompleted(request, 0);
  }

  void OnAuthRequired(net::URLRequest* request,
                      const net::AuthChallengeInfo& auth_info) override {
    std::cerr << "auth req" << std::endl;
  }

  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override {
    std::cerr << "cert req" << std::endl;
  }

  void OnSSLCertificateError(net::URLRequest* request,
                             int net_error,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override {
    std::cerr << "cert err" << std::endl;
  }

  void OnResponseStarted(net::URLRequest* request, int net_error) override {
    std::cerr << "resp started" << std::endl;
    // Get the header and send
    size_t iter = 0;
    std::string name;
    std::string value;
    std::vector<std::string> response_headers;
    response_headers.push_back(request->response_headers()->GetStatusLine() +
                               "\r\n");
    while (request->response_headers()->EnumerateHeaderLines(&iter, &name,
                                                             &value)) {
      // if(name.compare("Content-Encoding") == 0) continue;
      std::string h = name + ": " + value + "\r\n";
      response_headers.push_back(h);
    }
    for (std::string element : response_headers) {
      mg_send(connection_, &element[0], element.size());
      std::cerr << element << std::endl;
    }
    mg_send(connection_, "\r\n", 2);
    auto n = request->Read(buf_.get(), BUF_SZ);
    std::cerr << "resp read " << n << std::endl;

    if (n == net::ERR_IO_PENDING) {
      return;
    }
    OnReadCompleted(request, n);
  }

  void OnReadCompleted(net::URLRequest* request, int bytes_read) override {
    std::cerr << "completed" << std::endl;
    while (true) {
      std::cerr << "completed" << bytes_read << std::endl;
      if (bytes_read == net::ERR_IO_PENDING) {
        std::cerr << "PENDING" << std::endl;
        // usleep(3*microsecond);
        return;
      }
      if (bytes_read <= 0) {
        break;
      }
      // std::cout << std::string(buf_->data(), bytes_read) << std::endl;
      mg_send(connection_, buf_->data(), bytes_read);
      bytes_read = request->Read(buf_.get(), BUF_SZ);
    }
    std::cout << "closing connection" << std::endl;
    std::move(quit_closure_).Run();
  }

 private:
  base::OnceClosure quit_closure_;
  scoped_refptr<net::IOBuffer> buf_;
  struct mg_connection* connection_;
};

std::unique_ptr<net::UploadDataStream> CreateSimpleUploadData(
    const char* data) {
  auto reader =
      std::make_unique<net::UploadBytesElementReader>(data, strlen(data));
  return net::ElementsUploadDataStream::CreateWithReader(std::move(reader), 0);
}

void provider(struct mg_http_message* hm, struct mg_connection* c) {
  char* x[0];
  base::CommandLine::Init(0, x);
  auto args = base::CommandLine::ForCurrentProcess()->GetArgs();

  // Creating a thread
  base::AtExitManager exit_manager;
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Dowloaded");
  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::IO);
  base::RunLoop run_loop;

  // Building a context
  auto context_builder = std::make_unique<net::URLRequestContextBuilder>();

  context_builder->DisableHttpCache();
  auto ctx = context_builder->Build();
  auto quit_closure = run_loop.QuitClosure();
  MyDelegate delegate(quit_closure, c);

  // Copying value related to uri and method sent by mongoose
  struct mg_str* s = mg_http_get_header(hm, "X-Original-URL");
  char url[s->len];
  strncpy(url, s->ptr, s->len);
  url[s->len] = '\0';
  char method[hm->method.len ];
  strncpy(method, hm->method.ptr, hm->method.len);
  method[hm->method.len] = '\0';

  //std::cerr << "" << s->ptr;
  MG_INFO(("URL from X-ORIGINAL-URL  %s",s->ptr));
  // Create request
  auto req = ctx->CreateRequest(
      GURL(url), net::RequestPriority::DEFAULT_PRIORITY, &delegate);

  // Things to change here: CRITICAL
  MG_INFO(("Method  %s",method));
  size_t i, max = sizeof(hm->headers) / sizeof(hm->headers[0]);
  net::HttpRequestHeaders headers;
  for (i = 0; i < max && hm->headers[i].name.len > 0; i++) {
    struct mg_str *k = &hm->headers[i].name, *v = &hm->headers[i].value;
    if (mg_strcmp(*k, mg_str("X-Original-Url")) == 0) {
      continue;
    }
    char key[k->len+1];
    char value[v->len+1];
    memcpy(key, k->ptr, k->len);
    memcpy(value, v->ptr, v->len);
    key[k->len] = '\0';
    value[v->len] = '\0';
    //std::cerr << "key now " << key << std::endl;
  MG_INFO(("KEY  %s",key));
  MG_INFO(("VALUE  %s",value));
    //std::cerr << "value now " << value << std::endl;
    //headers.SetHeader(key, value);
    req->SetExtraRequestHeaderByName(key, value, true);
  }
  //req->SetExtraRequestHeaders(headers);

  // POST DATA
  if (hm->body.len > 0) {
    char body[hm->body.len+1];
    memcpy(body, hm->body.ptr, hm->body.len);
    body[hm->body.len] = '\0';
    //std::cerr << "body to upload " << body << std::endl;
    MG_INFO(("body to upload  %s",body));
    req->set_upload(CreateSimpleUploadData(body));
  }
  req->set_method(method);
  req->Start();

  run_loop.Run();
  base::ThreadPoolInstance::Get()->Shutdown();
  base::ThreadPoolInstance::Get()->JoinForTesting();
  base::ThreadPoolInstance::Set(nullptr);
}

// We use the same event handler function for HTTP and HTTPS connections
// fn_data is NULL for plain HTTP, and non-NULL for HTTPS
static void fn(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    MG_INFO(("body -> %.*s", (int)hm->body.len, hm->body.ptr));
    // Calling chromium
    if (mg_strcmp(hm->method, mg_str("CONNECT")) != 0) {
      provider(hm, c);
    } else {
      //  MG_INFO(("CONNECT encountered",hm->header));
    }
    // Closing connection
    c->is_draining = 1;
  }
  (void)fn_data;
}

int main(int argc, char* argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  struct mg_mgr mgr;                            // Event manager
  mg_log_set(MG_LL_DEBUG);                      // Set log level
  mg_mgr_init(&mgr);                    // Initialise event manager
  mg_http_listen(&mgr, s_http_addr, fn, NULL); 
   // Create HTTP listener
  while (s_signo == 0) {
    mg_mgr_poll(&mgr, 0);
  }
  mg_mgr_free(&mgr);
  MG_INFO(("Exiting on signal %d", s_signo));
  return 0;
}
