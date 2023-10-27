#ifndef CHROMIUM_SIMPLE_WEBSOCKET_CLIENT_H
#define CHROMIUM_SIMPLE_WEBSOCKET_CLIENT_H

#include "base/memory/scoped_refptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_runner.h"
#include "net/url_request/url_request_context.h"
#include "net/websockets/websocket_channel.h"
#include "net/websockets/websocket_errors.h"

#include "simple_websocket_event_interface.h"

class SimpleWebSocketClient {
public:
    SimpleWebSocketClient(const scoped_refptr<base::SingleThreadTaskRunner> &task_runner,
                          const std::string &url,
                          const std::string &proxy);

    virtual ~SimpleWebSocketClient();

    // Waits until request is done.
    bool WaitForDone(const base::TimeDelta& wait_delta) { return is_done_->TimedWait(wait_delta); }

    // Starts the closing handshake for a client-initiated shutdown of the connection.
    void Close(const std::string &reason, uint16_t code = net::kWebSocketNormalClosure);

    // Returns error message if OnFailed callback is invoked.
    std::string last_error_message() const { return last_error_message_; }

private:
    void Initialize(const std::string &url, const std::string &proxy);

    void Finalize();

    void ReadFrames();

    void StartClosingHandshake(uint16_t code, const std::string &reason);

private:
    std::string last_error_message_;

    std::unique_ptr<base::WaitableEvent> is_done_ = std::make_unique<base::WaitableEvent>();

    std::unique_ptr<net::WebSocketChannel> channel_;

    net::WebSocketChannel::ChannelState channel_State_ = net::WebSocketChannel::CHANNEL_ALIVE;

    std::unique_ptr<net::URLRequestContext> request_context_;

    raw_ptr<SimpleWebSocketEventInterface, DanglingUntriaged> handler_;  // owned by channel_

    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

#endif //CHROMIUM_SIMPLE_WEBSOCKET_CLIENT_H
