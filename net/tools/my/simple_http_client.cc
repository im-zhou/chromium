#include <iostream>
#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "net/cert/x509_certificate.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/url_request/url_request_context_builder.h"

#include "simple_http_client.h"

SimpleHTTPClient::SimpleHTTPClient(const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
                                   const std::string &url,
                                   const std::string &proxy,
                                   const std::string &method) : task_runner_(task_runner) {
    DCHECK(task_runner);
    bool res = task_runner->PostTask(FROM_HERE,
                                     base::BindOnce(&SimpleHTTPClient::Initialize,
                                                    base::Unretained(this),
                                                    std::move(url),
                                                    std::move(proxy),
                                                    std::move(method)));
    DCHECK(res);
}

SimpleHTTPClient::~SimpleHTTPClient() = default;

void SimpleHTTPClient::Cancel() {
    DCHECK(task_runner_);
    int res = task_runner_->PostTask(FROM_HERE,
                                     base::BindOnce(&SimpleHTTPClient::TriggerCancel,
                                                    base::Unretained(this)));
    DCHECK(res);

    is_done_->Reset();
}

void SimpleHTTPClient::Initialize(const std::string &url, const std::string &proxy, const std::string &method) {
    scoped_refptr<base::SingleThreadTaskRunner> task_runner = base::SingleThreadTaskRunner::GetCurrentDefault();
    DCHECK(task_runner->BelongsToCurrentThread());

    // Building a context
    auto context_builder = std::make_unique<net::URLRequestContextBuilder>();
    context_builder->DisableHttpCache();
    if (!proxy.empty()) {
        net::ProxyConfig proxy_config;
        proxy_config.proxy_rules().ParseFromString(proxy);
        std::unique_ptr<net::ProxyConfigService> proxy_config_service = std::make_unique<net::ProxyConfigServiceFixed>(
                net::ProxyConfigWithAnnotation(proxy_config,
                                               MISSING_TRAFFIC_ANNOTATION));
        context_builder->set_proxy_config_service(std::move(proxy_config_service));
    }
    request_context_ = context_builder->Build();

    // Create callback
    handler_ = std::make_unique<SimpleUrlRequestCallback>([&]() {
        scoped_refptr<base::SequencedTaskRunner> task_runner = base::SequencedTaskRunner::GetCurrentDefault();
        DCHECK(task_runner);
        int res = task_runner->PostTask(FROM_HERE,
                                        base::BindOnce(&SimpleHTTPClient::Finalize,
                                                       base::Unretained(this)));
        DCHECK(res);
    });

    // Create request
    request_ = request_context_->CreateRequest(
            GURL(url),
            net::RequestPriority::DEFAULT_PRIORITY,
            handler_.get(),
            MISSING_TRAFFIC_ANNOTATION);
    request_->set_method(method);
    request_->Start();
}

void SimpleHTTPClient::Finalize() {
    DCHECK_CALLED_ON_VALID_THREAD(handler_->getNetworkThreadChecker());

    last_error_message_ = handler_->last_error_message();
    response_as_string_ = handler_->response_as_string();

    request_ = nullptr;
    request_context_ = nullptr;
    handler_ = nullptr;

    is_done_->Signal();
}

void SimpleHTTPClient::TriggerCancel() {
    DCHECK_CALLED_ON_VALID_THREAD(handler_->getNetworkThreadChecker());

    request_->Cancel();
}
