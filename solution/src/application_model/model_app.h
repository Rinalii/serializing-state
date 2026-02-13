#pragma once

#include <random>
#include <unordered_map>
#include <memory>
#include <string>

#include "../domain_model/model_game.h"

namespace model {

class Player;
class GameSession;

class Player {
public:
    Player(const std::string& name) : name_(name), id_(++id_counter_){
        dog_ = std::make_shared<Dog>(id_);
    }
    Player(const Dog& dog, std::string name, int id) : name_(name), id_(id){
        dog_ = std::make_shared<Dog>(dog);
    }
    Player(const std::shared_ptr<Dog>& dog, std::string name, int id) : name_(name), id_(id){
        dog_ = dog;
    }
    void SetIdCounter(int id_counter) {
        id_counter_ = id_counter;
    }

    ~Player() {}

    const std::string& GetName() const {return name_;}
    int GetId() const noexcept {return id_;}
    std::shared_ptr<Dog> GetDog() const {return dog_;}
    const std::shared_ptr<GameSession> GetPlayersSession() const {return session_;}
    std::shared_ptr<GameSession> GetPlayersSession() {return session_;}

    void AddAndPrepareGameSession(std::shared_ptr<GameSession> session, std::shared_ptr<model::Map> map, bool is_rand_spawn);
    int GetIdCounter() const {
        return id_counter_;
    }
    void SetSession(const std::shared_ptr<GameSession>& session) {
        session_ = session;
    }
    
private:
    std::string name_;
    int id_;
    std::shared_ptr<Dog> dog_;
    
    std::shared_ptr<GameSession> session_;
    static int id_counter_;
};

}


