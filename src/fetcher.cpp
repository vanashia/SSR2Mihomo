#include "ssr2mihomo/fetcher.h"

#include <curl/curl.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace ssr2mihomo {
namespace {

std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    const std::size_t total = size * nmemb;
    auto* output = static_cast<std::string*>(userdata);
    output->append(ptr, total);
    return total;
}

}  // namespace

std::string choose_user_agent(const std::string& url, const std::string& override_user_agent) {
    if (!override_user_agent.empty()) {
        return override_user_agent;
    }

    const std::string lowered = [&url]() {
        std::string copy = url;
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return copy;
    }();

    if (lowered.find("shadowrocket=1") != std::string::npos) {
        return "Shadowrocket/1.0";
    }

    return
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";
}

std::string fetch_url(const std::string& url,
                      bool allow_insecure,
                      const std::string& override_user_agent) {
    CURL* handle = curl_easy_init();
    if (handle == nullptr) {
        throw std::runtime_error("failed to initialize curl");
    }

    std::string response;
    const std::string user_agent = choose_user_agent(url, override_user_agent);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");

    if (allow_insecure) {
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    const CURLcode code = curl_easy_perform(handle);
    if (code != CURLE_OK) {
        const std::string message = curl_easy_strerror(code);
        curl_easy_cleanup(handle);
        throw std::runtime_error("download failed: " + message);
    }

    long response_code = 0;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(handle);

    const bool is_file_url = url.rfind("file://", 0) == 0;
    if (!is_file_url && (response_code < 200 || response_code >= 300)) {
        throw std::runtime_error("unexpected HTTP status: " + std::to_string(response_code));
    }

    return response;
}

}  // namespace ssr2mihomo
