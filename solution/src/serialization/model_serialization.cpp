#include "model_serialization.h"


namespace model {
void RestoreSession(model::Game& game, const GameSessionReprTmp& session_repr) {
    std::string map_id_string = session_repr.GetMapIdString();
    const std::vector<PlayerReprTmp>& player_reprs = session_repr.GetPlayerRepr();
    const std::vector<LootObjectRepr>& loot_objerc_reprs = session_repr.GetLootsObjectRepr();

    const Map::Id map_id{map_id_string};

    std::shared_ptr<GameSession> curr_session = game.GetGameSession(map_id);
    for(const auto& loot_objerc_repr : loot_objerc_reprs) {
        model::LootObject loot_object = loot_objerc_repr.Restore();
        curr_session->AddLootObject(loot_object);
    }

    for(const auto& player_repr : player_reprs) {
        std::string player_name = player_repr.GetPlayerName();
        int player_id = player_repr.GetPlayerId();
        int id_counter_player = player_repr.GetIdCounter();

        const DogRepr& dog_repr = player_repr.GetDogRepr();
        model::Dog dog = dog_repr.Restore();
        std::shared_ptr<model::Dog> dog_ptr = std::make_shared<Dog>(dog);

        model::Player player(dog_ptr, player_name, player_id);
        player.SetIdCounter(id_counter_player);
        player.SetSession(curr_session);

        std::string token_string = player_repr.GetPlayerToken();
        const model::Token token{token_string};

        curr_session->AddDog(dog_ptr);
        game.AddRestoredPlayer(curr_session, player, token);
    }
}

void Restore(model::Game& game) {
    std::stringstream ss;
    boost::archive::text_iarchive ia{ss};

    std::vector<GameSessionReprTmp> game_ses_reprs;
    ia >> game_ses_reprs;

    for(auto session_repr : game_ses_reprs) {
        RestoreSession(game, session_repr);
    }
}

void Save(model::Game& game) {
    std::vector<GameSessionReprTmp> game_ses_reprs;

    const std::unordered_map<std::shared_ptr<GameSession>, PlayerTokens>& sessions = game.GetSessions();
    for(auto& [session_ptr, players_tokens] : sessions) {
        GameSessionReprTmp session_repr(*session_ptr, players_tokens);
        game_ses_reprs.push_back(session_repr);
    }

    std::stringstream ss;
    // Сериализация
    {
        boost::archive::text_oarchive oa{ss};
        oa << game_ses_reprs;
    }
}


void Restore(model::Game& game, std::string filename) {
    std::fstream in_fstream;
    in_fstream.open(filename, std::ios_base::in);

    if(!in_fstream.is_open()) {
        return;
    }

    boost::archive::text_iarchive ia{in_fstream};

    std::vector<GameSessionReprTmp> game_ses_reprs;
    ia >> game_ses_reprs;
    in_fstream.close();

    for(auto session_repr : game_ses_reprs) {
        RestoreSession(game, session_repr);
    }
}

void Save(model::Game& game, std::string filename) {
    std::vector<GameSessionReprTmp> game_ses_reprs;

    const std::unordered_map<std::shared_ptr<GameSession>, PlayerTokens>& sessions = game.GetSessions();
    for(auto& [session_ptr, players_tokens] : sessions) {
        GameSessionReprTmp session_repr(*session_ptr, players_tokens);
        game_ses_reprs.push_back(session_repr);
    }

    std::fstream out_fstream;
    out_fstream.open(filename, std::ios_base::out);

    if(!out_fstream.is_open()) {
        return;
    }

    boost::archive::text_oarchive oa{out_fstream};
    oa << game_ses_reprs;
    out_fstream.close();
}
}