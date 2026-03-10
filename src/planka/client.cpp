#include "client.h"
#include <cstdlib>
#include <workflow/HttpMessage.h>
#include <logger.h>

PlankaClient::PlankaClient(const Config& config) : config_(config) {
    if (!config_.url.empty()) {
        if (config_.url.find("://") == std::string::npos) {
            config_.url = "http://" + config_.url;
        }
        if (config_.url.back() == '/') {
            config_.url.pop_back();
        }
    }
}

PlankaClient PlankaClient::from_env() {
    Config conf;
    conf.url = get_env("PLANKA_URL");
    conf.api_key = get_env("PLANKA_API_KEY");
    return PlankaClient(conf);
}

std::string PlankaClient::get_env(const std::string& name) {
    const char* val = std::getenv(name.c_str());
    return val ? std::string(val) : "";
}

std::string PlankaClient::full_url(const std::string& path) const {
    std::string p = path;
    if (p.empty() || p[0] != '/') {
        p = "/" + p;
    }
    return config_.url + p;
}

void PlankaClient::add_auth_headers(coke::HttpClient::HttpHeader& headers) const {
    if (!config_.api_key.empty()) {
        headers.push_back({"X-Api-Key", config_.api_key});
    }
    headers.push_back({"Content-Type", "application/json"});
}

static std::string get_body(const protocol::HttpResponse& resp) {
    const void *body;
    size_t size;
    if (resp.get_parsed_body(&body, &size)) {
        return std::string(static_cast<const char*>(body), size);
    }
    return "";
}

coke::Task<wfrest::Json> PlankaClient::get(const std::string& path) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    std::string url = full_url(path);
    auto result = co_await http_client_.request(url, "GET", headers, "");
    
    if (result.state == coke::STATE_SUCCESS) {
        const char* status_str = result.resp.get_status_code();
        int status = status_str ? std::atoi(status_str) : 0;
        std::string body = get_body(result.resp);
        
        if (status < 200 || status >= 300) {
            LOG_ERROR() << "[PlankaClient] HTTP " << status << " for path: " << path << " Body: " << body;
            wfrest::Json err = wfrest::Json::Object();
            err.push_back("__is_error__", true);
            err.push_back("status", status);
            
            wfrest::Json body_json = wfrest::Json::parse(body);
            if (body_json.is_object()) {
                if (body_json.has("code")) err.push_back("code", body_json["code"]);
                if (body_json.has("message")) err.push_back("message", body_json["message"]);
                if (body_json.has("problems")) err.push_back("problems", body_json["problems"]);
            }
            
            if (!err.has("message")) {
                err.push_back("error", body);
                err.push_back("message", body);
            }
            co_return err;
        }
        
        LOG_DEBUG() << "[PlankaClient] " << path << " Status: " << status;
        co_return wfrest::Json::parse(body);
    }
    LOG_ERROR() << "[PlankaClient] Request failed for " << path << " state: " << (int)result.state;
    wfrest::Json err = wfrest::Json::Object();
    err.push_back("error", "Network or internal error");
    err.push_back("__is_error__", true);
    co_return err;
}

coke::Task<wfrest::Json> PlankaClient::post(const std::string& path, const wfrest::Json& body) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "POST", headers, body.dump());
    
    if (result.state == coke::STATE_SUCCESS) {
        const char* status_str = result.resp.get_status_code();
        int status = status_str ? std::atoi(status_str) : 0;
        std::string res_body = get_body(result.resp);
        if (status < 200 || status >= 300) {
            LOG_ERROR() << "[PlankaClient] POST " << path << " status: " << status << " Body: " << res_body;
            wfrest::Json err = wfrest::Json::Object();
            err.push_back("__is_error__", true);
            err.push_back("status", status);
            
            // Try to parse Planka error format
            wfrest::Json body_json = wfrest::Json::parse(res_body);
            if (body_json.is_object()) {
                if (body_json.has("code")) err.push_back("code", body_json["code"]);
                if (body_json.has("message")) err.push_back("message", body_json["message"]);
                if (body_json.has("problems")) err.push_back("problems", body_json["problems"]);
            }
            
            if (!err.has("message")) {
                err.push_back("error", res_body);
                err.push_back("message", res_body);
            }
            co_return err;
        }
        co_return wfrest::Json::parse(res_body);
    }
    wfrest::Json err = wfrest::Json::Object();
    err.push_back("error", "POST failed");
    err.push_back("__is_error__", true);
    co_return err;
}

coke::Task<wfrest::Json> PlankaClient::patch(const std::string& path, const wfrest::Json& body) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "PATCH", headers, body.dump());
    
    if (result.state == coke::STATE_SUCCESS) {
        const char* status_str = result.resp.get_status_code();
        int status = status_str ? std::atoi(status_str) : 0;
        std::string res_body = get_body(result.resp);
        if (status < 200 || status >= 300) {
            LOG_ERROR() << "[PlankaClient] PATCH " << path << " status: " << status << " Body: " << res_body;
            wfrest::Json err = wfrest::Json::Object();
            err.push_back("__is_error__", true);
            err.push_back("status", status);
            
            wfrest::Json body_json = wfrest::Json::parse(res_body);
            if (body_json.is_object()) {
                if (body_json.has("code")) err.push_back("code", body_json["code"]);
                if (body_json.has("message")) err.push_back("message", body_json["message"]);
                if (body_json.has("problems")) err.push_back("problems", body_json["problems"]);
            }
            
            if (!err.has("message")) {
                err.push_back("error", res_body);
                err.push_back("message", res_body);
            }
            co_return err;
        }
        co_return wfrest::Json::parse(res_body);
    }
    wfrest::Json err = wfrest::Json::Object();
    err.push_back("error", "PATCH failed");
    err.push_back("__is_error__", true);
    co_return err;
}

coke::Task<wfrest::Json> PlankaClient::del(const std::string& path) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "DELETE", headers, "");
    
    if (result.state == coke::STATE_SUCCESS) {
        const char* status_str = result.resp.get_status_code();
        int status = status_str ? std::atoi(status_str) : 0;
        std::string res_body = get_body(result.resp);
        if (status < 200 || status >= 300) {
            LOG_ERROR() << "[PlankaClient] DELETE " << path << " status: " << status << " Body: " << res_body;
            wfrest::Json err = wfrest::Json::Object();
            err.push_back("__is_error__", true);
            err.push_back("status", status);
            
            wfrest::Json body_json = wfrest::Json::parse(res_body);
            if (body_json.is_object()) {
                if (body_json.has("code")) err.push_back("code", body_json["code"]);
                if (body_json.has("message")) err.push_back("message", body_json["message"]);
                if (body_json.has("problems")) err.push_back("problems", body_json["problems"]);
            }
            
            if (!err.has("message")) {
                err.push_back("error", res_body);
                err.push_back("message", res_body);
            }
            co_return err;
        }
        co_return wfrest::Json::parse(res_body);
    }
    wfrest::Json err = wfrest::Json::Object();
    err.push_back("error", "DELETE failed");
    err.push_back("__is_error__", true);
    co_return err;
}
