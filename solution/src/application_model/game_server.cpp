#include "game_server.h"

std::shared_ptr<const model::Player> GameServer::FindPlayer(const model::Token& token) const {
    return game_.FindPlayer(token);
}

std::shared_ptr<model::Map> GameServer::FindMap(const model::Map::Id& id) const noexcept {
    return game_.FindMap(id);
}

const std::vector<model::Map>& GameServer::GetMaps() const noexcept {
    return game_.GetMaps();
}

void GameServer::Tick2(int tick) {
    int millisec_per_sec = 1000;
    game_.GenerateLoot(tick);
    game_.UpdateGame(static_cast<double>(tick)/millisec_per_sec);
}
void GameServer::Tick2(std::chrono::milliseconds delta) {
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