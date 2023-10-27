#ifndef CHROMIUM_SIMPLE_WEBSOCKET_EVENT_INTERFACE_H
#define CHROMIUM_SIMPLE_WEBSOCKET_EVENT_INTERFACE_H

#include "net/ssl/ssl_info.h"
#include "net/url_request/url_request.h"
#include "net/websockets/websocket_event_interface.h"
#include "net/websockets/websocket_handshake_request_info.h"
#include "net/websockets/websocket_handshake_response_info.h"

class SimpleWebSocketEventInterface : public net::WebSocketEventInterface {
public:
    typedef const std::function<void(void)> finalizer;
    typedef const std::function<void(void)> connected;

    explicit SimpleWebSocketEventInterface(const finalizer &finalizer, const connected &connected);

    ~SimpleWebSocketEventInterface() override;

    const base::ThreadChecker &getNetworkThreadChecker() const;

    // Returns error message if OnFailed callback is invoked.
    std::string last_error_message() const { return last_error_message_; }

    void OnCreateURLRequest(net::URLRequest *request) override;

    void OnAddChannelResponse(std::unique_ptr<net::WebSocketHandshakeResponseInfo> response,
                              const std::string &selected_subprotocol, const std::string &extensions) override;

    void OnDataFrame(bool fin, net::WebSocketEventInterface::WebSocketMessageType type,
                     base::span<const char> payload) override;

    bool HasPendingDataFrames() override;

    void OnSendDataFrameDone() override;

    void OnClosingHandshake() override;

    void OnDropChannel(bool was_clean, uint16_t code, const std::string &reason) override;

    void OnFailChannel(const std::string &message, int net_error, absl::optional<int> response_code) override;

    void OnStartOpeningHandshake(std::unique_ptr<net::WebSocketHandshakeRequestInfo> request) override;

    void OnSSLCertificateError(std::unique_ptr<SSLErrorCallbacks> ssl_error_callbacks, const GURL &url, int net_error,
                               const net::SSLInfo &ssl_info, bool fatal) override;

    int
    OnAuthRequired(const net::AuthChallengeInfo &auth_info, scoped_refptr<net::HttpResponseHeaders> response_headers,
                   const net::IPEndPoint &socket_address,
                   base::OnceCallback<void(const net::AuthCredentials *)> callback,
                   absl::optional<net::AuthCredentials> *credentials) override;

private:
    std::unique_ptr<base::ThreadChecker> network_thread_checker_;

    std::string last_error_message_;

    finalizer finalizer_;
    connected connected_;
};

#endif //CHROMIUM_SIMPLE_WEBSOCKET_EVENT_INTERFACE_H
