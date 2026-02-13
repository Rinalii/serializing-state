#pragma once

#include <filesystem>

#include "domain_model/model_game.h"
#include "application_model/game.h"

#include <boost/json.hpp>

namespace json_loader {

std::string LoadJsonFileAsString(const std::filesystem::path& json_path);
void AddRoadsToMap(const boost::json::value& json_roads, model::Map& map);
void AddBuildingsToMap(const boost::json::value& json_buildings, model::Map& map);
void AddOfficesToMap(const boost::json::value& json_offices, model::Map& map);
void AddMapsToGame (const boost::json::value& json_maps, model::Game& game, double default_dog_speed, int default_bag_capacity);
void AddLootTypesAtMap(const boost::json::object& json_map_obj, model::Map& map);
model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
