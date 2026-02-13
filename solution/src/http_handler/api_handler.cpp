#include "api_handler.h"
namespace http_handler {

using StringResponse = http::response<http::string_body>;
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, 
                                std::string_view content_type, 
                                std::string_view cache, 
                                std::string_view allow) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.keep_alive(keep_alive);
    if (!cache.empty()) {
        response.set(http::field::cache_control, cache);
    }
    if (!allow.empty()) {
        response.set(http::field::allow, allow);
    }
    response.prepare_payload();
    return response;
}

ApiObject DetermineApiObject(const std::string& target_str) {
    auto check_size_and_slash = [&target_str](size_t prefix_size, ApiObject matched_object) {
        if (target_str.size() == prefix_size) {
            return matched_object;
        }
        if (target_str.size() == prefix_size + 1 && target_str[prefix_size] == '/') {
            return matched_object;
        }
        return ApiObject::UNKNOWN;
    };

    if (target_str.starts_with(pattern_urls::MAPS)) {
        auto result = check_size_and_slash(pattern_urls::MAPS.size(), ApiObject::MAPS);
        if (result != ApiObject::UNKNOWN) {
            return result;
        }
        if (target_str.size() > pattern_urls::MAPS.size() && target_str[pattern_urls::MAPS.size()] == '/') {
            return ApiObject::MAPBYID;
        }
        return ApiObject::UNKNOWN;

    } else if (target_str.starts_with(pattern_urls::GAME_PLAYERS)) {
        return check_size_and_slash(pattern_urls::GAME_PLAYERS.size(), ApiObject::PLAYERS);
    } else if (target_str.starts_with(pattern_urls::GAME_JOIN)) {
        return check_size_and_slash(pattern_urls::GAME_JOIN.size(), ApiObject::JOIN);
    } else if (target_str.starts_with(pattern_urls::GAME_STATE)) {
        return check_size_and_slash(pattern_urls::GAME_STATE.size(), ApiObject::STATE);
    } else if (target_str.starts_with(pattern_urls::GAME_PLAYER_ACTION)) {
        return check_size_and_slash(pattern_urls::GAME_PLAYER_ACTION.size(), ApiObject::ACTION);
    } else if (target_str.starts_with(pattern_urls::GAME_TICK)) {
        return check_size_and_slash(pattern_urls::GAME_TICK.size(), ApiObject::TICK);
    }
    return ApiObject::UNKNOWN;
}

boost::json::array ApiRequestHandler::CreateRoadsJson(const model::Map& map) {
    boost::json::array json_roads;
    for (const auto& road : map.GetRoads()) {
        boost::json::object json_road;
        json_road["x0"] = road.GetStart().x;
        json_road["y0"] = road.GetStart().y;
        if(road.IsHorizontal()) {
            json_road["x1"] = road.GetEnd().x;
        }
        else {
            json_road["y1"] = road.GetEnd().y;
        }
        json_roads.push_back(json_road);
    }
    return json_roads;
}

boost::json::array ApiRequestHandler::CreateBuildingsJson(const model::Map& map) {
    boost::json::array json_buildings;
    for (const auto& building : map.GetBuildings()) {
        boost::json::object json_building;
        const model::Rectangle& building_param = building.GetBounds();

        json_building["x"] = building_param.position.x;
        json_building["y"] = building_param.position.y;
        json_building["w"] = building_param.size.width;
        json_building["h"] = building_param.size.height;

        json_buildings.push_back(json_building);
    }
    return json_buildings;
}

boost::json::array ApiRequestHandler::CreateOfficesJson(const model::Map& map) {
    boost::json::array json_offices;
    for (const auto& office : map.GetOffices()) {
        boost::json::object json_office;
        json_office["id"] = *(office.GetId());
        json_office["x"] = static_cast<int>(office.GetPosition().x);
        json_office["y"] = static_cast<int>(office.GetPosition().y);
        json_office["offsetX"] = office.GetOffset().dx;
        json_office["offsetY"] = office.GetOffset().dy;
        
        json_offices.push_back(json_office);
    }
    return json_offices;
}

boost::json::array ApiRequestHandler::CreateLootTypesJson(const model::Map& map) {
    boost::json::array json_loot_types;
    for (const auto& loot_type : map.GetLootTypes()) {
        boost::json::object json_loot_type;
        json_loot_type["name"] = loot_type.name;
        json_loot_type["file"] = loot_type.file;
        json_loot_type["type"] = loot_type.type;
        if(loot_type.rotation) {
            json_loot_type["rotation"] = *loot_type.rotation;
        }
        if(!loot_type.color.empty()) {
            json_loot_type["color"] = loot_type.color;
        }
        
        if(loot_type.scale) {
            json_loot_type["scale"] = *loot_type.scale;
        }

        json_loot_type["value"] = loot_type.value;
        
        json_loot_types.push_back(json_loot_type);
    }
    return json_loot_types;
}

std::string ApiRequestHandler::GetMapsResponseBody() const {
    boost::json::array json_maps;

    for(const auto& map : game_server_.GetMaps()) {
        boost::json::object json_map;
        json_map["id"] = *(map.GetId());
        json_map["name"] = map.GetName();
        json_maps.push_back(json_map);
    }

    return boost::json::serialize(json_maps);
}

std::string ApiRequestHandler::GetMapByIdResponseBody(std::string_view map_id) const {
    boost::json::object json_map_by_id;

    model::Map::Id  id{std::string(map_id)};
    std::shared_ptr<model::Map> map = game_server_.FindMap(id);
    if (map != nullptr) {
        json_map_by_id["id"] = *(map->GetId());
        json_map_by_id["name"] = map->GetName();
        json_map_by_id["roads"] = CreateRoadsJson(*map);
        json_map_by_id["buildings"] = CreateBuildingsJson(*map);
        json_map_by_id["offices"] = CreateOfficesJson(*map);
        json_map_by_id["lootTypes"] = CreateLootTypesJson(*map);
        return boost::json::serialize(json_map_by_id);
    }
    return "";
}

std::string ApiRequestHandler::GetJoinResponseBody(std::shared_ptr<model::Map> map, const std::string& user_name) const {
    try {
        std::pair<const std::shared_ptr<model::Player>, model::Token> player_and_token = game_server_.JoinGame(map, user_name);
        boost::json::object responce_body;
        responce_body["authToken"] = *player_and_token.second;
        responce_body["playerId"] = player_and_token.first->GetId();
        return boost::json::serialize(responce_body);
    } catch (const std::exception& ex) { 
        throw;
    }
}

std::string ApiRequestHandler::GetPlayerListResponseBody(std::shared_ptr<const model::Player> player) const {
    boost::json::object responce_body_obj;
    const std::shared_ptr<model::GameSession> session = player->GetPlayersSession();
    for (auto token_and_player : game_server_.GetTokenToPlayerMap(session)) {
        std::shared_ptr<model::Player> tmp_player = token_and_player.second;
        responce_body_obj[std::to_string(tmp_player->GetId())] = boost::json::object{{"name", tmp_player->GetName()}};
    }
    return boost::json::serialize(responce_body_obj);
}

std::string ApiRequestHandler::GetGameStateResponseBody(std::shared_ptr<const model::Player> player) const {
    boost::json::object responce_body_obj;
    boost::json::object players_json;
    boost::json::object loot_objects_json;
    const std::shared_ptr<model::GameSession> session = player->GetPlayersSession();
    for (auto token_and_player : game_server_.GetTokenToPlayerMap(session)) {
        auto& tmp_player = token_and_player.second;
        
        auto dog = tmp_player->GetDog();
        boost::json::object player_json;
        player_json["dir"] = dog->GetDirection();
        player_json["pos"] = {dog->GetPosition().x, dog->GetPosition().y};
        player_json["speed"] = {dog->GetSpeed().x, dog->GetSpeed().y};

        const model::Bag& bag = dog->GetBag();
        boost::json::array items_in_bag_json;

        for(const auto& item : bag.loot_objects) {
            boost::json::object item_object_json;
            item_object_json["id"] = item->GetId();
            item_object_json["type"] = item->GetType();
            
            items_in_bag_json.push_back(item_object_json);
        }

        player_json["bag"] = items_in_bag_json;
        player_json["score"] = dog->GetScore();

        players_json[std::to_string(tmp_player->GetId())] = player_json;
    }
    responce_body_obj["players"] = players_json;

    const std::unordered_map<int, std::shared_ptr<model::LootObject>>& loot_objects = session->GetLootObjects();

    for(const auto& [id, loot_object] : loot_objects) {
        boost::json::object loot_object_json;
        loot_object_json["type"] = loot_object->GetType();
        geom::Point2D position = loot_object->GetPosition();
        loot_object_json["pos"] = {static_cast<double>(position.x), static_cast<double>(position.y)};
        loot_objects_json[std::to_string(id)] = loot_object_json;
    }

    responce_body_obj["lostObjects"] = loot_objects_json;
    return boost::json::serialize(responce_body_obj);
}

void ApiRequestHandler::DoPlayerAction(std::shared_ptr<const model::Player> player, const std::string& direction) const {
    try {
        player->GetDog()->SetDirection(direction);
    } catch (const std::exception& ex) {
        throw;
    }
}

}  // namespace http_handler