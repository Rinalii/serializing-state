#pragma once
#include "../domain_model/model_game.h"
#include "player_tokens.h"

namespace model {

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    std::shared_ptr<GameSession> GetGameSession(const Map::Id& id);
    std::shared_ptr<GameSession> GetGameSession(std::shared_ptr<Map> map);
    void PrintMaps() const;
    void UpdateGame(double dt);

    std::shared_ptr<GameSession> CreateGameSession(const Map::Id& id) {
        auto map = FindMap(id);

        auto game_session = std::make_shared<GameSession>(map);
        game_sessions_to_players_tok_[game_session];

        return game_session;
    }

    std::shared_ptr<GameSession> GetGameSessionOrNullptr(const Map::Id& id) {
        for(auto& [session, _] : game_sessions_to_players_tok_) {
            if(session->GetMap()->GetId() == id) {
                return session;
            }
        }
        return nullptr;
    }
    Game(double base_interval = 0.5, double probability = 0.5)
        : loot_generator_(
            std::chrono::milliseconds(static_cast<long long>(base_interval * 1000)),
            probability)
    {}

    void GenerateLoot(double time_delta_sec) {
        
        for(auto& [session, _] : game_sessions_to_players_tok_) {
            const std::vector<std::shared_ptr<Dog>> dogs = session->GetDogs();
            int loot_count = session->GetSizeLootObjects();
            unsigned looter_count = dogs.size();
            unsigned number = loot_generator_.Generate(std::chrono::milliseconds(static_cast<long long>(time_delta_sec * 1000))
                            , loot_count, looter_count);
            session->GenerateLootObjects(number);
        }
    }

    std::pair<std::shared_ptr<model::Player>, model::Token> JoinGame(std::shared_ptr<model::Map> map, const std::string& player_name, bool is_rand_spawn){
        std::shared_ptr<model::GameSession> session = GetGameSession(map);
        if (!session) {
            throw std::runtime_error("Failed to create game session.");
        }

        std::shared_ptr<model::Player> player = std::make_shared<model::Player>(player_name);
        player->AddAndPrepareGameSession(session, map, is_rand_spawn);
        auto token = game_sessions_to_players_tok_[session].AddPlayer(*player);

        return {player, token};
    }

    std::shared_ptr<const model::Player> FindPlayer(const model::Token& token) const {
        for(auto& [_, players_to_tokens] : game_sessions_to_players_tok_) {
            std::shared_ptr<const model::Player> player = players_to_tokens.FindPlayer(token);
            if(player) {
                return player;
            }
        }
        return nullptr;
    }

    std::shared_ptr<const model::Player> FindPlayer(int id) const {
        for(auto& [_, players_to_tokens] : game_sessions_to_players_tok_) {
            std::shared_ptr<const model::Player> player = players_to_tokens.FindPlayer(id);
            if(player) {
                return player;
            }
        }
        return nullptr;
    }

    const std::unordered_map<model::Token, std::shared_ptr<model::Player>, model::TokenHasher> GetTokenToPlayerMap(std::shared_ptr<model::GameSession> session) const noexcept {
        auto it = game_sessions_to_players_tok_.find(session);
        if(it != game_sessions_to_players_tok_.end()) {
            return it->second.GetTokenToPlayerMap();
        }
        return std::unordered_map<model::Token, std::shared_ptr<model::Player>, model::TokenHasher>();
    }

    const std::unordered_map<std::shared_ptr<GameSession>, PlayerTokens>& GetSessions() const {
        return game_sessions_to_players_tok_;
    }
private:
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, util::TaggedHasher<Map::Id>>;

    MapIdToIndex map_id_to_index_;
    std::vector<Map> maps_;
    std::unordered_map<std::shared_ptr<GameSession>, PlayerTokens> game_sessions_to_players_tok_;

    loot_gen::LootGenerator loot_generator_;
};

}