#include "game.h"
#include <iostream>

using namespace std::literals;

namespace model {
    void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept  {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return std::make_shared<Map>(maps_.at(it->second));
    }
    return nullptr;
}

std::shared_ptr<GameSession> Game::GetGameSession(const Map::Id& id) {
    auto map = FindMap(id);
    return GetGameSession(map);
}

std::shared_ptr<GameSession> Game::GetGameSession(std::shared_ptr<Map> map) {
    if (map == nullptr) {
        throw std::invalid_argument("Map doesn't exist");
    }
    const auto& id = map->GetId();

    for(auto& [session, _] : game_sessions_to_players_tok_) {
        if(session->GetMap()->GetId() == id) {
            return session;
        }
    }

    auto game_session = std::make_shared<GameSession>(map);
    game_sessions_to_players_tok_[game_session];

    return game_session;
}

void Game::PrintMaps() const {
    for (Map map : maps_) {
        std::cout << "Map: " << map.GetName() << std::endl;
        for (auto road : map.GetRoads()) {
            std::cout << "{" << road.GetStart().x << ", " << road.GetStart().y << "} - {" << road.GetEnd().x << ", " << road.GetEnd().y << "}" << std::endl;
        }
        std::cout << std::endl;
    }
}

void Game::UpdateGame(double dt) {
    for(auto& [game_session, _] : game_sessions_to_players_tok_) {
        game_session->UpdateDogsPosition(dt);
    }
}
}