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

#include "simple_websocket_client.h"

SimpleWebSocketClient::SimpleWebSocketClient(const scoped_refptr<base::SingleThreadTaskRunner> &task_runner,
                                             const std::string &url,
                                             const std::string &proxy) : task_runner_(task_runner) {
    DCHECK(task_runner_);
    bool res = task_runner_->PostTask(FROM_HERE,
                                      base::BindOnce(&SimpleWebSocketClient::Initialize,
                                                     base::Unretained(this),
                                                     std::move(url),
                                                     std::move(proxy)));
    DCHECK(res);
}

SimpleWebSocketClient::~SimpleWebSocketClient() = default;

void SimpleWebSocketClient::Close(const std::string &reason, uint16_t code) {
    DCHECK(task_runner_);
    int res = task_runner_->PostTask(FROM_HERE,
                                     base::BindOnce(&SimpleWebSocketClient::StartClosingHandshake,
                                                    base::Unretained(this),
                                                    code,
                                                    std::move(reason)));
    DCHECK(res);

    is_done_->Reset();
}

void SimpleWebSocketClient::Initialize(const std::string &url, const std::string &proxy) {
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

    // Create callbacks
    auto event_interface = std::make_unique<SimpleWebSocketEventInterface>(
            [&]() {
                scoped_refptr<base::SequencedTaskRunner> task_runner = base::SequencedTaskRunner::GetCurrentDefault();
                DCHECK(task_runner);
                int res = task_runner->PostTask(FROM_HERE,
                                                base::BindOnce(&SimpleWebSocketClient::Finalize,
                                                               base::Unretained(this)));
                DCHECK(res);
            }, [&]() {
                scoped_refptr<base::SequencedTaskRunner> task_runner = base::SequencedTaskRunner::GetCurrentDefault();
                DCHECK(task_runner);
                int res = task_runner->PostTask(FROM_HERE,
                                                base::BindOnce(&SimpleWebSocketClient::ReadFrames,
                                                               base::Unretained(this)));
                DCHECK(res);
            });
    handler_ = event_interface.get();

    // Create channel
    std::vector<std::string> sub_protocols;
    GURL socket_url = GURL(url);
    url::Origin origin = url::Origin::Create(socket_url);
    net::SiteForCookies site_for_cookies = net::SiteForCookies::FromOrigin(origin);
    net::IsolationInfo isolation_info = net::IsolationInfo::Create(
            net::IsolationInfo::RequestType::kOther,
            origin, origin, net::SiteForCookies::FromOrigin(origin));
    net::HttpRequestHeaders additional_headers;

    channel_ = std::make_unique<net::WebSocketChannel>(
            std::move(event_interface),
            request_context_.get());
    channel_->SendAddChannelRequest(
            socket_url,
            sub_protocols,
            origin,
            site_for_cookies,
            false,
            isolation_info,
            additional_headers,
            MISSING_TRAFFIC_ANNOTATION);
}

void SimpleWebSocketClient::Finalize() {
    DCHECK_CALLED_ON_VALID_THREAD(handler_->getNetworkThreadChecker());

    last_error_message_ = handler_->last_error_message();

    channel_ = nullptr;
    request_context_ = nullptr;
    handler_ = nullptr;

    is_done_->Signal();
}

void SimpleWebSocketClient::ReadFrames() {
    DCHECK_CALLED_ON_VALID_THREAD(handler_->getNetworkThreadChecker());

    channel_State_ = channel_->ReadFrames();
}

void SimpleWebSocketClient::StartClosingHandshake(uint16_t code, const std::string &reason) {
    DCHECK_CALLED_ON_VALID_THREAD(handler_->getNetworkThreadChecker());

    if (channel_State_ == net::WebSocketChannel::CHANNEL_ALIVE) {
        channel_State_ = channel_->StartClosingHandshake(code, reason);
    }
}
