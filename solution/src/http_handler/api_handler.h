#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <variant>

#include "../domain_model/tagged.h"
#include "../domain_model/model_env.h"
#include "../application_model/model_app.h"
#include "../application_model/game_server.h"

namespace json = boost::json;
namespace beast = boost::beast;
namespace http = beast::http;

namespace http_handler{

enum class ApiObject {
    MAPS,
    MAPBYID,
    PLAYERS,
    JOIN,
    STATE,
    ACTION,
    TICK,
    UNKNOWN
};

namespace content_type {
    inline constexpr static std::string_view HTML = "text/html"sv;
    inline constexpr static std::string_view CSS = "text/css"sv;
    inline constexpr static std::string_view TXT = "text/plain"sv;
    inline constexpr static std::string_view JS = "text/javascript"sv;

    inline constexpr static std::string_view JSON = "application/json"sv;
    inline constexpr static std::string_view XML = "application/xml"sv;

    inline constexpr static std::string_view PNG = "image/png"sv;
    inline constexpr static std::string_view JPG = "image/jpeg"sv;
    inline constexpr static std::string_view GIF = "image/gif"sv;
    inline constexpr static std::string_view BMP = "image/bmp"sv;
    inline constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
    inline constexpr static std::string_view TIFF = "image/tiff"sv;
    inline constexpr static std::string_view SVG = "image/svg+xml"sv;

    inline constexpr static std::string_view MP3 = "audio/mpeg"sv;
    inline constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
}

namespace pattern_urls {
    inline constexpr static std::string_view MAPS = "/api/v1/maps"sv;
    inline constexpr static std::string_view GAME_PLAYERS = "/api/v1/game/players"sv;
    inline constexpr static std::string_view GAME_JOIN = "/api/v1/game/join"sv;
    inline constexpr static std::string_view GAME_STATE = "/api/v1/game/state"sv;
    inline constexpr static std::string_view GAME_PLAYER_ACTION = "/api/v1/game/player/action"sv;
    inline constexpr static std::string_view GAME_TICK = "/api/v1/game/tick"sv;
}

namespace errors_handler {
    inline constexpr static std::string_view BAD_REQ = R"({"code": "badRequest", "message": "Bad request"})"sv;

    inline constexpr static std::string_view MAP_NOT_FOUND = R"({"code": "mapNotFound", "message": "Map not found"})"sv;
    inline constexpr static std::string_view USERNAME_EMPTY = R"({"code": "invalidArgument", "message": "Invalid name"})"sv;
    inline constexpr static std::string_view PARSING_ERROR = R"({"code": "invalidArgument", "message": "Join game request parse error"})"sv;

    inline constexpr static std::string_view INVALID_POST = R"({"code": "invalidMethod", "message": "Only POST method is expected"})"sv;
    inline constexpr static std::string_view AUTH_HEADER = R"({"code": "invalidToken", "message": "Authorization header is missing"})"sv;
    inline constexpr static std::string_view UNKNOWN_TOKEN = R"({"code": "unknownToken", "message": "Player token has not been found"})"sv;
    inline constexpr static std::string_view INVALID_METHOD = R"({"code": "invalidMethod", "message": "GET or HEAD method is expected"})"sv;

    inline constexpr static std::string_view INVALID_GET = R"({"code": "invalidMethod", "message": "Only GET method is expected"})"sv;
    inline constexpr static std::string_view INVALID_TOKEN = R"({"code": "invalidToken", "message": "Player token is invalid"})"sv;

    inline constexpr static std::string_view ACTION_PARSING_ERROR = R"({"code": "invalidArgument", "message": "Failed to parse action"})"sv;
    inline constexpr static std::string_view TICK_PARSING_ERROR = R"({"code": "invalidArgument", "message": "Failed to parse tick request JSON"})"sv; 

    inline constexpr static std::string_view MAP_ID_EMPTY = R"({"code": "invalidArgument", "message": "Invalid map id"})"sv;
    inline constexpr static std::string_view BAD_REQ_TICK = R"({"code": "badRequest", "message": "Invalid endpoint"})"sv;
}

using StringResponse = http::response<http::string_body>;
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, 
                                std::string_view content_type = content_type::HTML, std::string_view cache = ""sv, std::string_view allow =""sv);

ApiObject DetermineApiObject(const std::string& target_str);

class ApiRequestHandler {
        using Response = std::variant<http::response<http::string_body>, http::response<http::file_body>>;
public:
    explicit ApiRequestHandler(GameServer& game_server) : game_server_{game_server} {}

    ApiRequestHandler(const ApiRequestHandler&) = delete;
    ApiRequestHandler& operator=(const ApiRequestHandler&) = delete;
    ~ApiRequestHandler() {}

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandleRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& req_target) {

        ApiObject api_object = DetermineApiObject(req_target);
        std::string map_id;

        switch(api_object) {
            case ApiObject::MAPS:
                return HandleMapsRequest(req);
            case ApiObject::MAPBYID:
                map_id = req_target.substr(pattern_urls::MAPS.size()+1);
                return HandleMapByIdRequest(req, map_id);
            case ApiObject::PLAYERS:
                return HandlePlayersListRequest(req);
            case ApiObject::JOIN:
                return HandlePlayerJoinRequest(req);
            case ApiObject::STATE:
                return HandleGameStateRequest(req);
            case ApiObject::ACTION:
                return HandlePlayerActionRequest(req);
            case ApiObject::TICK:
                if(!game_server_.IsAutoTick()) {
                    return HandleTickRequest(req);
                }
                return MakeStringResponse(http::status::bad_request, errors_handler::BAD_REQ_TICK, req.version(), req.keep_alive(), content_type::JSON);
            default:
                break;
        }
        return MakeStringResponse(http::status::bad_request, errors_handler::BAD_REQ, req.version(), req.keep_alive(), content_type::JSON);
    }

private:
    GameServer& game_server_;

    static boost::json::array CreateRoadsJson(const model::Map& map);
    static boost::json::array CreateBuildingsJson(const model::Map& map);
    static boost::json::array CreateOfficesJson(const model::Map& map);
    static boost::json::array CreateLootTypesJson(const model::Map& map);

    std::string GetMapsResponseBody() const;
    std::string GetMapByIdResponseBody(std::string_view map_id) const;
    std::string GetJoinResponseBody(std::shared_ptr<model::Map> map, const std::string& user_name) const;
    std::string GetPlayerListResponseBody(std::shared_ptr<const model::Player> player) const;
    std::string GetGameStateResponseBody(std::shared_ptr<const model::Player> player) const;
    void DoPlayerAction(std::shared_ptr<const model::Player> player, const std::string& direction) const;

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandleMapsRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        
        if (req.method() != http::verb::get) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_GET, "GET"sv);
        }
        std::string responce_body = GetMapsResponseBody();
        return text_response(http::status::ok, responce_body);
    }

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandleMapByIdRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& map_id) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_METHOD, "GET, HEAD"sv);
        }
        if (map_id.empty()) {
            return text_response(http::status::bad_request, errors_handler::BAD_REQ);
        }

        std::string responce_body = GetMapByIdResponseBody(map_id);
        if(responce_body.empty()) {
            return text_response(http::status::not_found, errors_handler::MAP_NOT_FOUND);
        }
        return text_response(http::status::ok, responce_body);
    }
    template <typename Body, typename Allocator>
    http::response<http::string_body> HandlePlayerJoinRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv, std::string_view content_type = content_type::JSON) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type, "no-cache"sv, allow);
        };
        if (req.method() != http::verb::post) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_POST, "POST"sv);
        }

        std::string user_name;
        std::string map_id;
        try {
            auto parsed_req_obj = boost::json::parse(req.body()).as_object();
            if (!parsed_req_obj.contains("userName")) {
                return text_response(http::status::bad_request, errors_handler::USERNAME_EMPTY);
            }

            user_name = parsed_req_obj.at("userName").as_string();
            if (user_name.empty()) {
                return text_response(http::status::bad_request, errors_handler::USERNAME_EMPTY);
            }
            if (!parsed_req_obj.contains("mapId")) {
                return text_response(http::status::bad_request, errors_handler::MAP_ID_EMPTY);
            }

            map_id = parsed_req_obj.at("mapId").as_string();
            if (map_id.empty()) {
                return text_response(http::status::bad_request, errors_handler::MAP_ID_EMPTY);
            }
        } catch (const std::exception& ex) {
            return text_response(http::status::bad_request, errors_handler::PARSING_ERROR);
        }

        try{
            auto map = game_server_.FindMap(model::Map::Id{map_id});
            if (map == nullptr) {
                return text_response(http::status::not_found, errors_handler::MAP_NOT_FOUND);
            }

            std::string responce_body = GetJoinResponseBody(map, user_name);
            return text_response(http::status::ok, responce_body);
        } catch (const std::exception& ex) {
            return text_response(http::status::internal_server_error, "Join game failed: "s + ex.what(), "", content_type::HTML);
        }
    }

    template <typename Body, typename Allocator>
    std::optional<model::Token> TryExtractToken(const http::request<Body, http::basic_fields<Allocator>>& req) {

        auto it = req.find(http::field::authorization);
        if(it == req.end()) {
            return std::nullopt;
        }
        std::string authorization(it->value());

        const std::string auth_pattern = "Bearer ";
        if (authorization.starts_with(auth_pattern)) {
            authorization = authorization.substr(auth_pattern.size());
        } else {
            return std::nullopt;
        }

        std::transform(authorization.begin(), authorization.end(), authorization.begin(), [](unsigned char c) {return std::tolower(c);});
        
        if (authorization.size() != 32) {
            return std::nullopt;
        }                                                                                    
        if (std::any_of(authorization.begin(), authorization.end(), [](auto c){return !isxdigit(c);})) {
            return std::nullopt;
        }
        return model::Token(authorization);
    }

    template <typename Fn, typename Body, typename Allocator>
    http::response<http::string_body> ExecuteAuthorized(Fn&& action, const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv);
        };

        if (!req.count(http::field::authorization)) {
            return text_response(http::status::unauthorized, errors_handler::AUTH_HEADER);
        }

        auto token = this->TryExtractToken(req);
        if (token) {
            std::shared_ptr<const model::Player> player = game_server_.FindPlayer(*token);
            if (player == nullptr) {
                return text_response(http::status::unauthorized, errors_handler::UNKNOWN_TOKEN);
            }
            return action(player);
        }
        return text_response(http::status::unauthorized, errors_handler::INVALID_TOKEN);
    }

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandlePlayersListRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_METHOD, "GET, HEAD"sv);         
        } 
        
        return ExecuteAuthorized([this, &req, &text_response](std::shared_ptr<const model::Player> player) {
                    std::string responce_body = GetPlayerListResponseBody(player);
                    return text_response(http::status::ok, responce_body);
                }, req);
    }

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandleGameStateRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_METHOD, "GET, HEAD"sv);     
        } 

        return ExecuteAuthorized([this, &req, &text_response](std::shared_ptr<const model::Player> player) {
            std::string responce_body = GetGameStateResponseBody(player);
            return text_response(http::status::ok, responce_body);
        }, req);
    }

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandlePlayerActionRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        if (req.method() != http::verb::post) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_POST, "POST"sv);        
        } 
        return ExecuteAuthorized([this, &req, &text_response](std::shared_ptr<const model::Player> player) {
            try {
                json::value parsed_request = json::parse(req.body());
                std::string direction = static_cast<std::string>(parsed_request.as_object().at("move").as_string());
                DoPlayerAction(player, direction);
                return text_response(http::status::ok, "{}");
            } catch (const std::exception& ex) {
                return text_response(http::status::bad_request, errors_handler::ACTION_PARSING_ERROR);
            }
        }, req); 
    }

    template <typename Body, typename Allocator>
    http::response<http::string_body> HandleTickRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const auto text_response = [this, &req](http::status status, std::string_view text, std::string_view allow = ""sv) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type::JSON, "no-cache"sv, allow);
        };
        if (req.method() != http::verb::post) {
            return text_response(http::status::method_not_allowed, errors_handler::INVALID_POST, "POST"sv);         
        }

        try {
            auto parsed_req_obj = json::parse(req.body()).as_object();
            if (!parsed_req_obj.contains("timeDelta")) {
                return text_response(http::status::bad_request, errors_handler::BAD_REQ);
            }

            int dt = parsed_req_obj.at("timeDelta").as_int64();
            game_server_.Tick(dt);
            return text_response(http::status::ok, "{}");
        } catch (const std::exception& ex) {
            return text_response(http::status::bad_request, errors_handler::TICK_PARSING_ERROR);
        }
    }
};

}  // namespace http_handler

