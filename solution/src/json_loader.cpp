#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>

#include <iostream>

using namespace std::literals;

namespace json_loader {

std::string LoadJsonFileAsString(const std::filesystem::path& json_path) {
    std::ifstream json_file;
    json_file.open(json_path);

    if (!json_file.is_open()) {
        std::string error_message = "Failed to open file: "s + json_path.string();
        throw std::runtime_error(error_message);
    }

    std::stringstream buffer;
    buffer << json_file.rdbuf();
    json_file.close();

    return buffer.str();
}

void AddRoadsToMap(const boost::json::value& json_roads, model::Map& map) {
    for (auto& json_road : json_roads.as_array()) {
        const boost::json::object& json_road_obj = json_road.as_object();
        int x0 = static_cast<int>(json_road_obj.at(model_constants::X0).as_int64());
        int y0 = static_cast<int>(json_road_obj.at(model_constants::Y0).as_int64());

        if (json_road_obj.contains(model_constants::X1)) {
            int x1 = static_cast<int>(json_road_obj.at(model_constants::X1).as_int64());
            model::Road road{model::Road::HORIZONTAL, {x0, y0}, x1};
            map.AddRoad(road);
        } else {
            int y1 = static_cast<int>(json_road_obj.at(model_constants::Y1).as_int64());
            model::Road road{model::Road::VERTICAL, {x0, y0}, y1};
            map.AddRoad(road);
        }
    }
}

void AddBuildingsToMap(const boost::json::value& json_buildings, model::Map& map) {
    for (auto& json_building : json_buildings.as_array()) {
        const boost::json::object& json_building_obj = json_building.as_object();

        int x = static_cast<int>(json_building_obj.at(model_constants::X).as_int64());
        int y = static_cast<int>(json_building_obj.at(model_constants::Y).as_int64());
        int w = static_cast<int>(json_building_obj.at(model_constants::W).as_int64());
        int h = static_cast<int>(json_building_obj.at(model_constants::H).as_int64());
        model::Rectangle rect{{x, y}, {w, h}};
        model::Building building{rect};
        map.AddBuilding(building);
    }
}

void AddOfficesToMap(const boost::json::value& json_offices, model::Map& map) {
    for (auto& json_office : json_offices.as_array()) {
        const boost::json::object& json_office_obj = json_office.as_object();
        model::Office::Id id{json_office_obj.at("id").as_string().c_str()};
        int x = static_cast<int>(json_office_obj.at(model_constants::X).as_int64());
        int y = static_cast<int>(json_office_obj.at(model_constants::Y).as_int64());
        int dx = static_cast<int>(json_office_obj.at(model_constants::OFFSET_X).as_int64());
        int dy = static_cast<int>(json_office_obj.at(model_constants::OFFSET_Y).as_int64());
       
        model::Office office{id, {x, y}, {dx, dy}};
        map.AddOffice(office);
    }
}

void AddMapsToGame (const boost::json::value& json_maps, model::Game& game, double default_dog_speed, int default_bag_capacity = 3) {

    for (auto& json_map : json_maps.as_array()) {
        const boost::json::object& json_map_obj = json_map.as_object();
        model::Map::Id id{json_map_obj.at("id").as_string().c_str()};
        model::Map map = model::Map{id, json_map_obj.at("name").as_string().c_str()};

        if (json_map_obj.contains("dogSpeed")) {
            map.SetDogSpeed(json_map_obj.at("dogSpeed").as_double());
        } else map.SetDogSpeed(default_dog_speed);

        if (json_map_obj.contains("bagCapacity")) {
            map.SetBagCapacity(json_map_obj.at("bagCapacity").as_int64());
        } else map.SetBagCapacity(default_bag_capacity);

        AddLootTypesAtMap(json_map_obj, map);
        AddRoadsToMap(json_map_obj.at("roads").as_array(), map);
        AddBuildingsToMap(json_map_obj.at("buildings").as_array(), map);
        AddOfficesToMap(json_map_obj.at("offices").as_array(), map);

        map.AddRoadIndexes();
       
        game.AddMap(map);
    }
}

void AddLootTypesAtMap(const boost::json::object& json_map_obj, model::Map& map) {
    std::vector<model::LootType> loot_types;
    if (json_map_obj.contains("lootTypes")) {
        const boost::json::array& json_loot_types_arr = json_map_obj.at("lootTypes").as_array();

        for (auto& json_loot_type : json_loot_types_arr) {
            const boost::json::object& json_loot_type_obj = json_loot_type.as_object();
            model::LootType loot_type;
            if (json_loot_type_obj.contains("name")) {
                loot_type.name = json_loot_type_obj.at("name").as_string().c_str();
            }
            if (json_loot_type_obj.contains("file")) {
                loot_type.file = json_loot_type_obj.at("file").as_string().c_str();
            }
            if (json_loot_type_obj.contains("type")) {
                loot_type.type = json_loot_type_obj.at("type").as_string().c_str();
            } 
            if (json_loot_type_obj.contains("rotation")) {
                loot_type.rotation = json_loot_type_obj.at("rotation").as_int64();
            } 
            std::string color;
            if (json_loot_type_obj.contains("color")) {
                loot_type.color = json_loot_type_obj.at("color").as_string().c_str();
            } 
            double scale;
            if (json_loot_type_obj.contains("scale")) {
                loot_type.scale = json_loot_type_obj.at("scale").as_double();
            } 
            int value = 0;
            if (json_loot_type_obj.contains("value")) {
                loot_type.value = json_loot_type_obj.at("value").as_int64();
            } 
            loot_types.push_back(loot_type);
        }

        map.SetLootTypes(loot_types);
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    std::string json_as_string = LoadJsonFileAsString(json_path);
    boost::json::value parsed_json = boost::json::parse(json_as_string);

    double period = 1.;
    double probability = 0.;
    if (parsed_json.as_object().contains("lootGeneratorConfig")) {
        boost::json::value loot_generator_config = parsed_json.as_object().at("lootGeneratorConfig");
        if (loot_generator_config.as_object().contains("period")) {
            period = loot_generator_config.as_object().at("period").as_double();
        }
        if (loot_generator_config.as_object().contains("probability")) {
            probability = loot_generator_config.as_object().at("probability").as_double();
        }
    }

    model::Game game(period, probability);

    double default_dog_speed = 1.;
    if (parsed_json.as_object().contains("defaultDogSpeed")) {
        default_dog_speed = parsed_json.as_object().at("defaultDogSpeed").as_double();
    }

    int default_bag_capacity = 3;
    if (parsed_json.as_object().contains("defaultBagCapacity")) {
        default_bag_capacity = parsed_json.as_object().at("defaultBagCapacity").as_int64();
    }

    AddMapsToGame(parsed_json.as_object().at("maps").as_array(), game, default_dog_speed, default_bag_capacity);
    return game;
}

}  // namespace json_loader
