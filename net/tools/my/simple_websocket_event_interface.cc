#include <iostream>
#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "net/cert/x509_certificate.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/ssl/ssl_cert_request_info.h"

#include "simple_websocket_event_interface.h"

SimpleWebSocketEventInterface::SimpleWebSocketEventInterface(const finalizer &finalizer, const connected &connected) :
        network_thread_checker_(std::make_unique<base::ThreadChecker>()),
        finalizer_(finalizer),
        connected_(connected) {
}

SimpleWebSocketEventInterface::~SimpleWebSocketEventInterface() = default;

const base::ThreadChecker &SimpleWebSocketEventInterface::getNetworkThreadChecker() const {
    return network_thread_checker_.operator*();
}

void SimpleWebSocketEventInterface::OnCreateURLRequest(net::URLRequest *request) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnCreateURLRequest called: "
              << request->url() << std::endl;
}

void SimpleWebSocketEventInterface::OnAddChannelResponse(std::unique_ptr<net::WebSocketHandshakeResponseInfo> response,
                                                         const std::string &selected_subprotocol,
                                                         const std::string &extensions) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnAddChannelResponse called: "
              << response->url << (selected_subprotocol.empty() ? "" : "\n")
              << selected_subprotocol << (extensions.empty() ? "" : "\n")
              << extensions << std::endl;

    connected_();
}

void SimpleWebSocketEventInterface::OnDataFrame(bool fin, net::WebSocketEventInterface::WebSocketMessageType type,
                                                base::span<const char> payload) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnDataFrame called: "
              << std::string(payload.begin(), payload.end()) << std::endl;
}

bool SimpleWebSocketEventInterface::HasPendingDataFrames() {
    return false;
}

void SimpleWebSocketEventInterface::OnSendDataFrameDone() {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnSendDataFrameDone called." << std::endl;
}

void SimpleWebSocketEventInterface::OnClosingHandshake() {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnClosingHandshake called." << std::endl;
}

void SimpleWebSocketEventInterface::OnDropChannel(bool was_clean, uint16_t code, const std::string &reason) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnDropChannel called: "
              << (was_clean ? "was clean" : "not clean") << ", code = " << code << ", " << reason <<
              std::endl;

    finalizer_();
}

void SimpleWebSocketEventInterface::OnFailChannel(const std::string &message, int net_error,
                                                  absl::optional<int> response_code) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnFailChannel called: "
              << net::ErrorToString(net_error)
              << ", " << message << (response_code.has_value() ? "\n" : "")
              << (response_code.has_value() ? std::to_string(response_code.value()) : "")
              << std::endl;

    last_error_message_ = net::ErrorToString(net_error);

    finalizer_();
}

void
SimpleWebSocketEventInterface::OnStartOpeningHandshake(std::unique_ptr<net::WebSocketHandshakeRequestInfo> request) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnStartOpeningHandshake called: "
              << request->url << std::endl;
}

void SimpleWebSocketEventInterface::OnSSLCertificateError(std::unique_ptr<SSLErrorCallbacks> ssl_error_callbacks,
                                                          const GURL &url, int net_error,
                                                          const net::SSLInfo &ssl_info, bool fatal) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnSSLCertificateError called: "
              << ssl_info.cert->subject().GetDisplayName() << std::endl;

    last_error_message_ = net::ErrorToString(net_error);

    ssl_error_callbacks->CancelSSLRequest(net_error, &ssl_info);

    finalizer_();
}

int
SimpleWebSocketEventInterface::OnAuthRequired(const net::AuthChallengeInfo &auth_info,
                                              scoped_refptr<net::HttpResponseHeaders> response_headers,
                                              const net::IPEndPoint &socket_address,
                                              base::OnceCallback<void(const net::AuthCredentials *)> callback,
                                              absl::optional<net::AuthCredentials> *credentials) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    return net::OK;
}

