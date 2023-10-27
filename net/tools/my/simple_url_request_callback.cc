#include <iostream>
#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "net/cert/x509_certificate.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/url_request/url_request_context_builder.h"

#include "simple_url_request_callback.h"

SimpleUrlRequestCallback::SimpleUrlRequestCallback(const finalizer &finalizer, const size_t &buffer_size) :
        network_thread_checker_(std::make_unique<base::ThreadChecker>()),
        buffer_size_(buffer_size),
        finalizer_(finalizer) {
}

SimpleUrlRequestCallback::~SimpleUrlRequestCallback() = default;

const base::ThreadChecker &SimpleUrlRequestCallback::getNetworkThreadChecker() const {
    return network_thread_checker_.operator*();
}

void SimpleUrlRequestCallback::OnReceivedRedirect(net::URLRequest *request,
                                                  const net::RedirectInfo &redirect_info,
                                                  bool *defer_redirect) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnRedirectReceived called: "
              << redirect_info.new_url << std::endl;
}

void SimpleUrlRequestCallback::OnCertificateRequested(net::URLRequest *request,
                                                      net::SSLCertRequestInfo *cert_request_info) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnCertificateRequested called: "
              << cert_request_info->host_and_port.ToString() << std::endl;

    request->ContinueWithCertificate(nullptr, nullptr);
}

void
SimpleUrlRequestCallback::OnSSLCertificateError(net::URLRequest *request,
                                                int net_error,
                                                const net::SSLInfo &ssl_info,
                                                bool fatal) {
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnSSLCertificateError called: "
              << ssl_info.cert->subject().GetDisplayName() << std::endl;

    last_error_message_ = net::ErrorToString(net_error);

    request->Cancel();

    finalizer_();
}

void SimpleUrlRequestCallback::OnResponseStarted(net::URLRequest *request,
                                                 int net_error) {
    DCHECK_NE(net::ERR_IO_PENDING, net_error);
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    if (net_error != net::OK) {
        last_error_message_ = net::ErrorToString(net_error);
        finalizer_();
        return;
    }

    std::cout << "OnResponseStarted called: ";
    std::cout << "HTTP Status: "
              << request->GetResponseCode() << " "
              << request->response_headers()->GetStatusText() << std::endl;

    read_buffer_ = base::MakeRefCounted<net::IOBuffer>(buffer_size_);
    int result = request->Read(read_buffer_.get(), buffer_size_);
    // If IO is pending, wait for the URLRequest to call OnReadCompleted.
    if (result == net::ERR_IO_PENDING)
        return;

    OnReadCompleted(request, result);
}

void
SimpleUrlRequestCallback::OnReadCompleted(net::URLRequest *request, int bytes_read) {
    DCHECK(read_buffer_);
    DCHECK_CALLED_ON_VALID_THREAD(getNetworkThreadChecker());

    std::cout << "OnReadCompleted called" << std::endl;

    if (bytes_read < 0) {
        last_error_message_ = net::ErrorToString(bytes_read);
        finalizer_();
        return;
    } else if (bytes_read == 0) {
        DCHECK(last_error_message_.empty());
    } else {
        response_as_string_.append(read_buffer_->data(), bytes_read);
    }

    int result = request->Read(read_buffer_.get(), buffer_size_);
    // If IO is pending, wait for the URLRequest to call OnReadCompleted.
    if (result == net::ERR_IO_PENDING)
        return;

    finalizer_();
}


