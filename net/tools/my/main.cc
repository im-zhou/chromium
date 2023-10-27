#include <iostream>
#include <string>
#include <regex>

#include "base/time/time.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"

#include "simple_http_client.h"
#include "simple_websocket_client.h"

bool matches(const std::string &url, const std::string &regexp) {
    std::regex re(regexp, std::regex_constants::icase);
    return std::regex_match(url.c_str(), re);
}

void handleHTTP(const scoped_refptr<base::SingleThreadTaskRunner> &network_task_runner,
                const std::string &url,
                const std::string &proxy) {
    SimpleHTTPClient client(network_task_runner, url, proxy);
    if (!client.WaitForDone(base::Seconds(3))) {
        client.Cancel();
        client.WaitForDone(base::Seconds(3));
    }

    if (client.last_error_message().empty()) {
        std::cout << "Response Data:" << std::endl
                  << client.response_as_string() << std::endl;
    } else {
        std::cout << "Response Error: "
                  << client.last_error_message() << std::endl;
    }
}

void handleWebsocket(const scoped_refptr<base::SingleThreadTaskRunner> &network_task_runner,
                     const std::string &url,
                     const std::string &proxy) {
    SimpleWebSocketClient client(network_task_runner, url, proxy);
    if (!client.WaitForDone(base::Seconds(30))) {
        client.Close("Bye");
        client.WaitForDone(base::Seconds(3));
    }

    if (!client.last_error_message().empty()) {
        std::cout << "Channel Error: "
                  << client.last_error_message() << std::endl;
    }
}

int main(int argc, const char *argv[]) {
    std::string url(argc > 1 ? argv[1] : "https://www.example.com");
    std::cout << "URL: " << url << std::endl;
    std::string proxy(argc > 2 ? argv[2] : "");
    if (!proxy.empty()) {
        std::cout << "Proxy: " << proxy << std::endl;
    }

    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Main");
    base::ScopedClosureRunner cleanup(base::BindOnce([] { base::ThreadPoolInstance::Get()->Shutdown(); }));

    std::unique_ptr<base::Thread> network_thread = std::make_unique<base::Thread>("network");
    base::Thread::Options options;
    options.message_pump_type = base::MessagePumpType::IO;
    network_thread->StartWithOptions(std::move(options));
    scoped_refptr<base::SingleThreadTaskRunner> network_task_runner = network_thread->task_runner();

    if (matches(url, "^http.*$")) {
        handleHTTP(network_task_runner, url, proxy);
    } else if (matches(url, "^ws.*$")) {
        handleWebsocket(network_task_runner, url, proxy);
    } else {
        std::cerr << "Unknown protocol" << std::endl;
    }

    return 0;
}
