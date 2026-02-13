#pragma once

#include <random>
#include <unordered_map>
#include <memory>
#include <string>

#include "../domain_model/model_game.h"
#include "model_app.h"

namespace model {

class Player;

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;


class PlayerTokens {
public:

    PlayerTokens() {}
    ~PlayerTokens() {}


    std::shared_ptr<Player> FindPlayer(int id) const {
        for(auto [token, player] : token_to_player_) {
            if(id == player->GetId()){
                return player;
            }
        }
        return nullptr;
    }
    std::shared_ptr<Player> FindPlayer(const Token& token) const {
        auto it = token_to_player_.find(token);
        if(it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }
    Token AddPlayer(const Player& player) {
        Token token = GetToken();
        token_to_player_[token] = std::make_shared<Player>(player);
        return token;
    }

    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetTokenToPlayerMap() const {
        return token_to_player_;
    }

private:
    
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
    //Token GetToken();
    Token GetToken() {
        std::stringstream ss;
        ss << std::setw(16) << std::setfill('0') << std::hex << generator1_(); 
        ss << std::setw(16) << std::setfill('0') << std::hex << generator2_(); 
        std::string result = ss.str();
        assert(result.size() == 32);
        return Token(ss.str());
    }
}; 




}