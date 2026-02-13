#pragma once
#include "../json_loader.h"
#include "../domain_model/model_game.h"
#include <boost/asio/io_context.hpp>
#include <utility>
#include "model_app.h"
#include <boost/signals2.hpp>
#include <chrono>
#include <iostream>
#include "player_tokens.h"
#include "game.h"

#include "../serialization/model_serialization.h"

namespace sig = boost::signals2;
using milliseconds = std::chrono::milliseconds;


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
    using TickSignal = sig::signal<void(milliseconds delta)>;

    GameServer(fs::path config) :
        game_{json_loader::LoadGame(config)} {
    }

    std::pair<std::shared_ptr<model::Player>, model::Token> JoinGame(std::shared_ptr<model::Map> map, const std::string& player_name) {
        return game_.JoinGame(map, player_name, is_rand_spawn_);
    }

    std::shared_ptr<const model::Player> FindPlayer(const model::Token& token) const;
    std::shared_ptr<const model::Player> FindPlayer(int id) const {
        return game_.FindPlayer(id);
    }

    const std::unordered_map<model::Token, std::shared_ptr<model::Player>, model::TokenHasher> GetTokenToPlayerMap(std::shared_ptr<model::GameSession> session) const noexcept {
        return game_.GetTokenToPlayerMap(session);
    }

    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept;

    const std::vector<model::Map>& GetMaps() const noexcept;

    void Tick2(int tick);
    void Tick2(std::chrono::milliseconds delta);

    void SetRandSpawn();
    void SetAutoTick();
    bool IsAutoTick() const noexcept;
    bool IsRandomSpawn() const noexcept;

    // Добавляем обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }

    void Tick(milliseconds delta) {
        // Уведомляем подписчиков сигнала tick
        int millisec_per_sec = 1000;
        game_.GenerateLoot(delta.count());
        game_.UpdateGame(static_cast<double>(delta.count())/millisec_per_sec);
        tick_signal_(delta);
    }

    void Tick(int tick) {
        int millisec_per_sec = 1000;
        game_.GenerateLoot(tick);
        game_.UpdateGame(static_cast<double>(tick)/millisec_per_sec);
        milliseconds delta(tick);
        tick_signal_(delta);
    }

    void SetStateFile(std::string state_file) {
        state_file_ = state_file;
    }

    void SetSaveStatePeriod(unsigned int save_state_period) {
        save_state_period_ = save_state_period;
    }

    void Save() {
        model::Save(game_, state_file_);
    }

private:
    model::Game game_;

    bool is_rand_spawn_ = false;
    bool is_auto_tick_ = false;

    TickSignal tick_signal_;
    std::string state_file_;
    unsigned int save_state_period_ = 0;
};
