#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <chrono>
#include <variant>

#include "api_handler.h"
#include "../http_server.h"

#include "../logger.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;

enum class RequestType {
    STATIC,
    API
};

using FileResponse = http::response<http::file_body>;
FileResponse MakeFileResponse(http::status status, fs::path path, unsigned http_version,
                                bool keep_alive, std::string_view content_type = content_type::HTML, std::string_view cache_control =""sv);

std::string UrlDecode(const std::string& str);
std::string_view GetContentType(fs::path path);
bool IsSubPath(fs::path path, fs::path base);

RequestType DetermineRequestType(const std::string& target_str);

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(fs::path root, Strand api_strand, GameServer& game_server)
        : root_{std::move(root)}
        , api_strand_{api_strand}
        , game_server_(game_server)
        , api_handler_(std::make_shared<ApiRequestHandler>(game_server)) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        std::string req_target = UrlDecode(std::string(req.target()));
        RequestType req_type = DetermineRequestType(req_target);

        try {
            if (req_type == RequestType::API) {
                auto handle = [self = shared_from_this(), send,
                               req = std::forward<decltype(req)>(req), version, keep_alive, req_target] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        return send(self->HandleApiRequest(req, req_target));
                        //return send(self->api_handler_->HandleRequest(req, req_target));
                    } catch (const std::exception& ex) {
                        send(self->ReportServerError(version, keep_alive, ex.what()));
                    } catch (...) {
                        send(self->ReportServerError(version, keep_alive, "Unknown server error"));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                HandleFileRequest(req, req_target));
        } catch (const std::exception& ex) {
            send(ReportServerError(version, keep_alive, ex.what()));
        } catch (...) {
            send(ReportServerError(version, keep_alive, "Unknown server error"));
        }
    }

private:
    fs::path root_;
    Strand api_strand_;
    GameServer& game_server_;
    std::shared_ptr<ApiRequestHandler> api_handler_;

    using FileRequestResult = std::variant<StringResponse, FileResponse>;

    template <typename Body, typename Allocator>
    FileRequestResult HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>& req, const std::string& req_target) {
        const auto text_response = [this, &req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::TXT, ""sv, ""sv);
        };

        std::string filepath_string = root_.string() + req_target;
        if(fs::is_directory(filepath_string)) {
            filepath_string += "index.html"s;
        }

        if(!IsSubPath(filepath_string, root_)) {
            json::object about_error {{"code", "badRequest"},{"message", "File " + filepath_string + " outside root dir"}};
            return text_response(http::status::bad_request, json::serialize(about_error));
        }
        try{
            fs::path filepath = fs::weakly_canonical(filepath_string);
            std::string_view content_type = GetContentType(filepath);
            return MakeFileResponse(http::status::ok, filepath, req.version(), req.keep_alive(), content_type);
        } catch(std::exception& ex) {
            json::object about_error {{"code", "notFound"},{"message", "File " + filepath_string + " not found"}};
            return text_response(http::status::not_found, json::serialize(about_error));
        }
    }

    template <typename Body, typename Allocator>
    StringResponse HandleApiRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& req_target){
        return api_handler_->HandleRequest(req, req_target);
    }
    StringResponse ReportServerError(unsigned version, bool keep_alive){
        return MakeStringResponse(http::status::bad_request, errors_handler::BAD_REQ, version, keep_alive, content_type::JSON, "no-cache"sv);
    }
    StringResponse ReportServerError(unsigned version, bool keep_alive, const std::string& what){
        json::object about_error {{"code", "badRequest"},{"message", what}};
        return MakeStringResponse(http::status::bad_request, json::serialize(about_error), version, keep_alive, content_type::JSON, "no-cache"sv);
    }
}; 



template <typename RequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(RequestHandler& handler) : decorated_(handler) {}
    
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto start_time = std::chrono::steady_clock::now();
        std::string request_uri = static_cast<std::string>(req.target());
        std::string request_host = static_cast<std::string>(req.at(http::field::host));
        request_host = request_host.substr(0, request_host.rfind(':'));
        std::string request_method = static_cast<std::string>(req.method_string());

        LogRequest(request_uri, request_host, request_method);

        decorated_(std::move(req), [send = std::move(send), start_time, request_uri](auto&& response) {

            std::string content_type = "null";
            if (response.find(http::field::content_type) != response.end()) {
                content_type = std::string(response.at(http::field::content_type));
            }
            send(response);
            auto end_time = std::chrono::steady_clock::now();
            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

            LogResponse(duration_us, response.result_int(), content_type, request_uri);                
        });
    }

private:
    RequestHandler& decorated_;

    static void LogRequest(const std::string& host, const std::string& uri, const std::string& method) {
        boost::json::object obj{
            {"ip", host},
            {"URI", uri},
            {"method", method}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, obj) << "request received"sv;
    }

    static void LogResponse(int delta, int code, std::string content, std::string uri) {
        if(content.empty()){
            content = "null";
        }
        boost::json::object obj;
        obj = {
            {"uri", uri},
            {"response_time", delta},
            {"code", code},
            {"content_type", content}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, obj) << "response sent"sv;
    }
};


}
