#ifndef CHROMIUM_SIMPLE_URL_REQUEST_CALLBACK_H
#define CHROMIUM_SIMPLE_URL_REQUEST_CALLBACK_H

#include <iostream>
#include <string>

#include "base/threading/thread_checker.h"
#include "net/base/io_buffer.h"
#include "net/ssl/ssl_info.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"

class SimpleUrlRequestCallback : public net::URLRequest::Delegate {
public:
    typedef const std::function<void(void)> finalizer;

    explicit SimpleUrlRequestCallback(const finalizer &finalizer, const size_t &buffer_size = 32 * 1024);

    ~SimpleUrlRequestCallback() override;

    const base::ThreadChecker &getNetworkThreadChecker() const;

    // Returns error message if OnFailed callback is invoked.
    std::string last_error_message() const { return last_error_message_; }

    // Returns string representation of the received response.
    std::string response_as_string() const { return response_as_string_; }

    void OnReceivedRedirect(net::URLRequest *request,
                            const net::RedirectInfo &redirect_info,
                            bool *defer_redirect) override;

    void OnCertificateRequested(net::URLRequest *request,
                                net::SSLCertRequestInfo *cert_request_info) override;

    void
    OnSSLCertificateError(net::URLRequest *request,
                          int net_error,
                          const net::SSLInfo &ssl_info,
                          bool fatal) override;

    void OnResponseStarted(net::URLRequest *request,
                           int net_error) override;

    void OnReadCompleted(net::URLRequest *request,
                         int bytes_read) override;

private:
    std::unique_ptr<base::ThreadChecker> network_thread_checker_;

    scoped_refptr<net::IOBuffer> read_buffer_;

    size_t buffer_size_;

    std::string last_error_message_;

    std::string response_as_string_;

    finalizer finalizer_;
};

#endif //CHROMIUM_SIMPLE_URL_REQUEST_CALLBACK_H
