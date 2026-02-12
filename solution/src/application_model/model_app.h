#pragma once

#include <random>
#include <unordered_map>
#include <memory>
#include <string>

#include "../domain_model/model_game.h"

namespace model {

class Player;
class GameSession;

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class Player {
public:
    Player(const std::string& name) : name_(name), id_(++id_counter_){
        dog_ = std::make_shared<Dog>();
    }

    ~Player() {}

    const std::string& GetName() const {return name_;}
    int GetId() const noexcept {return id_;}
    std::shared_ptr<Dog> GetDog() const {return dog_;}
    const std::shared_ptr<GameSession> GetPlayersSession() const {return session_;}
    std::shared_ptr<GameSession> GetPlayersSession() {return session_;}

    void AddAndPrepareGameSession(std::shared_ptr<GameSession> session, std::shared_ptr<model::Map> map, bool is_rand_spawn);
    
private:
    std::string name_;
    int id_;
    std::shared_ptr<Dog> dog_;
    
    std::shared_ptr<GameSession> session_;
    static int id_counter_;
};

class PlayerTokens {
public:

    PlayerTokens() {}
    ~PlayerTokens() {}

    std::shared_ptr<Player> FindPlayer(const Token& token) const;
    Token AddPlayer(const Player& player);
    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetTokenToPlayerMap() const;
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
    Token GetToken();
}; 

}


