#pragma once

#include "../json_loader.h"
#include "../domain_model/model_game.h"
#include "model_app.h"

#include <boost/asio/io_context.hpp>

#include <utility>

namespace net = boost::asio;
namespace fs = std::filesystem;

using namespace std::literals;

class GameServer {
    GameServer() = delete;
    GameServer(const GameServer&) = delete;
    GameServer(GameServer&&) = delete;
    GameServer& operator=(const GameServer&) = delete;
    GameServer& operator=(GameServer&&) = delete;

public:
    GameServer(fs::path config) :
        game_{json_loader::LoadGame(config)} {
    }

    std::pair<std::shared_ptr<model::Player>, model::Token> JoinGame(std::shared_ptr<model::Map> map, const std::string& player_name);
    std::shared_ptr<const model::Player> FindPlayer(const model::Token& token) const;
    const std::unordered_map<model::Token, std::shared_ptr<model::Player>, model::TokenHasher>& GetTokenToPlayerMap() const noexcept;
    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept;
    const std::vector<model::Map>& GetMaps() const noexcept;

    void Tick(int tick);
    void Tick(std::chrono::milliseconds delta);
    void SetRandSpawn();
    void SetAutoTick();
    bool IsAutoTick() const noexcept;
    bool IsRandomSpawn() const noexcept;

private:
    model::Game game_;
    model::PlayerTokens player_tokens_;

    bool is_rand_spawn_ = false;
    bool is_auto_tick_ = false;
};
