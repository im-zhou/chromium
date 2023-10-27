#ifndef CHROMIUM_SIMPLE_HTTP_CLIENT_H
#define CHROMIUM_SIMPLE_HTTP_CLIENT_H

#include <iostream>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_runner.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"

#include "simple_url_request_callback.h"

class SimpleHTTPClient {
public:
    SimpleHTTPClient(const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
                     const std::string &url,
                     const std::string &proxy,
                     const std::string &method = "GET");

    virtual ~SimpleHTTPClient();

    // Waits until request is done.
    bool WaitForDone(const base::TimeDelta &wait_delta) { return is_done_->TimedWait(wait_delta); }

    // This method may be called to cancel the request.
    void Cancel();

    // Returns error message if OnFailed callback is invoked.
    std::string last_error_message() const { return last_error_message_; }

    // Returns string representation of the received response.
    std::string response_as_string() const { return response_as_string_; }

private:
    void Initialize(const std::string &url, const std::string &proxy, const std::string &method);

    void Finalize();

    void TriggerCancel();

private:
    std::string last_error_message_;

    std::string response_as_string_;

    std::unique_ptr<base::WaitableEvent> is_done_ = std::make_unique<base::WaitableEvent>();

    std::unique_ptr<net::URLRequest> request_;

    std::unique_ptr<net::URLRequestContext> request_context_;

    std::unique_ptr<SimpleUrlRequestCallback> handler_;

    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

#endif //CHROMIUM_SIMPLE_HTTP_CLIENT_H
