#include "game_server.h"


std::pair<std::shared_ptr<model::Player>, model::Token> GameServer::JoinGame(std::shared_ptr<model::Map> map, const std::string& player_name){
    std::shared_ptr<model::GameSession> session = game_.GetGameSession(map);
    if (!session) {
        throw std::runtime_error("Failed to create game session.");
    }

    std::shared_ptr<model::Player> player = std::make_shared<model::Player>(player_name);
    player->AddAndPrepareGameSession(session, map, is_rand_spawn_);
    auto token = player_tokens_.AddPlayer(*player);

    return {player, token};
}

std::shared_ptr<const model::Player> GameServer::FindPlayer(const model::Token& token) const {
    return player_tokens_.FindPlayer(token);
}

const std::unordered_map<model::Token, std::shared_ptr<model::Player>, model::TokenHasher>& GameServer::GetTokenToPlayerMap() const noexcept {
    return player_tokens_.GetTokenToPlayerMap();
}

std::shared_ptr<model::Map> GameServer::FindMap(const model::Map::Id& id) const noexcept {
    return game_.FindMap(id);
}

const std::vector<model::Map>& GameServer::GetMaps() const noexcept {
    return game_.GetMaps();
}

void GameServer::Tick(int tick) {
    int millisec_per_sec = 1000;
    game_.GenerateLoot(tick);
    game_.UpdateGame(static_cast<double>(tick)/millisec_per_sec);
}
void GameServer::Tick(std::chrono::milliseconds delta) {
    int millisec_per_sec = 1000;
    game_.GenerateLoot(delta.count());
    game_.UpdateGame(static_cast<double>(delta.count())/millisec_per_sec);
}

void GameServer::SetRandSpawn() {
    is_rand_spawn_ = true;
}
void GameServer::SetAutoTick() {
    is_auto_tick_ = true;
}
bool GameServer::IsAutoTick() const noexcept {return is_auto_tick_;}
bool GameServer::IsRandomSpawn() const noexcept {return is_auto_tick_;}
