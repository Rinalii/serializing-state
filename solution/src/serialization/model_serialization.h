#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../domain_model/geom.h"
#include "../domain_model/model_env.h"
#include "../domain_model/model_game.h"
#include "../application_model/model_app.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


#include "../domain_model/tagged.h"
#include "../application_model/player_tokens.h"

#include "../application_model/game.h"
#include <fstream>

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model{

template <typename Archive>
void serialize(Archive& ar, Point& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, PointDouble& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Size& size, [[maybe_unused]] const unsigned version) {
    ar& size.width;
    ar& size.height;
}

template <typename Archive>
void serialize(Archive& ar, Rectangle& rectangle, [[maybe_unused]] const unsigned version) {
    ar& rectangle.position;
    ar& rectangle.size;
}

template <typename Archive>
void serialize(Archive& ar, Offset& offset, [[maybe_unused]] const unsigned version) {
    ar& offset.dx;
    ar& offset.dy;
}

}


namespace collision_detector{

    template <typename Archive>
    void serialize(Archive& ar, Item& item, [[maybe_unused]] const unsigned version) {
        ar& item.position;
        ar& item.width;
    }

    template <typename Archive>
    void serialize(Archive& ar, Gatherer& gatherer, [[maybe_unused]] const unsigned version) {
        ar& gatherer.start_pos;
        ar& gatherer.end_pos;
        ar& gatherer.width;
    }

}

namespace model{




class LootObjectRepr {
public:
    LootObjectRepr() = default;

    explicit LootObjectRepr(const model::LootObject& loot_object)
        : item_(loot_object.GetPosition(), loot_object.GetWidth())
        , id_(loot_object.GetId())
        , type_(loot_object.GetType())
        , value_(loot_object.GetValue())
        , id_counter_(loot_object.GetIdCounter()) {
    }

    [[nodiscard]] model::LootObject Restore() const {
        model::LootObject loot_object(type_, value_, item_.position, item_.width);
        loot_object.SetId(id_);
        loot_object.SetIdCounter(id_counter_);
        return loot_object;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& item_;
        ar& id_;
        ar& type_;
        ar& value_;
        ar& id_counter_;
    }

private:
    collision_detector::Item item_;
    int id_;
    int type_ = 0;
    int value_ = 0;
    int id_counter_;
};

class BagRepr {
public:
    BagRepr() = default;

    explicit BagRepr(const model::Bag& bag)
        : capacity_(bag.capacity){
        for(auto loot_obj_ptr : bag.loot_objects) {
            loot_objects_repr_.emplace_back(*loot_obj_ptr);
        }
    }

    [[nodiscard]] model::Bag Restore() const {
        model::Bag bag;
        bag.capacity = capacity_;
        for(auto obj_repr_el : loot_objects_repr_) {
            auto loot_object = obj_repr_el.Restore();
            bag.AddLoot(std::make_shared<LootObject>(loot_object));
        }
        return bag;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& capacity_;
        ar& loot_objects_repr_;
    }

private:
    int capacity_ = 3;
    std::vector<LootObjectRepr> loot_objects_repr_;
};


template <typename Archive>
void serialize(Archive& ar, Direction& direction, [[maybe_unused]] const unsigned version) {
    ar& direction;
}

class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , id_counter_(dog.GetIdCounter())
        , position_(model::PointDouble{dog.GetPosition().x, dog.GetPosition().y})
        , speed_(model::PointDouble{dog.GetSpeed().x, dog.GetSpeed().y})
        , direction_(dog.GetDirectionEnum())
        , speed_value_(dog.GetSpeedValue())
        , bag_repr_(dog.GetBag())
        , score_(dog.GetScore())
        , gatherer_(collision_detector::Gatherer(dog.start_pos, dog.end_pos, dog.width)){

    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog(id_);
        dog.SetId(id_);
        dog.SetIdCounter(id_counter_);
        dog.SetPosition(position_);
        dog.SetSpeedValue(speed_value_);
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.SetGatherer(gatherer_.start_pos, gatherer_.end_pos);
        dog.SetWidth(gatherer_.width);
        
        Bag bag = bag_repr_.Restore();
        dog.SetBag(bag);

        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& id_counter_;
        ar& position_;
        ar& speed_;
        ar& direction_;
        ar& speed_value_;
        ar& bag_repr_;
        ar& score_;
        ar& gatherer_;
    }

private:
    int id_;
    int id_counter_;

    PointDouble position_;
    PointDouble speed_;
    Direction direction_=Direction::NORTH;

    double speed_value_ = 0.;

    BagRepr bag_repr_;
    int score_ = 0;
    collision_detector::Gatherer gatherer_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& game_session)
        : map_id_str_(*game_session.GetMap()->GetId()) {
        for(auto dog : game_session.GetDogs()) {
            DogRepr dog_repr(*dog);
            dogs_repr_.push_back(dog_repr);
        }
        for(auto [id, loot_object_ptr] : game_session.GetLootObjects()) {
            LootObjectRepr loot_object_repr(*loot_object_ptr);
            loot_objects_repr_.push_back(loot_object_repr);
        }

    }

    [[nodiscard]] std::string GetMapIdString() const {
        return map_id_str_;
    }
    [[nodiscard]] const std::vector<DogRepr>& GetDogsRepr() const {
        return dogs_repr_;
    }
    [[nodiscard]] const std::vector<LootObjectRepr>& GetLootsObjectRepr() const {
        return loot_objects_repr_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id_str_;
        ar& dogs_repr_;
        ar& loot_objects_repr_;
    }

private:
    std::string map_id_str_;
    std::vector<DogRepr> dogs_repr_;
    std::vector<LootObjectRepr> loot_objects_repr_;
};


class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const model::Player& player)
        : name_(player.GetName())
        , id_(player.GetId())
        , dog_repr_(*player.GetDog())    
        , session_repr_(*player.GetPlayersSession())
        , id_counter_(player.GetIdCounter()){
    }

    [[nodiscard]] std::string GetPlayerName() const {
        return name_;
    }
    [[nodiscard]] int GetPlayerId() const {
        return id_;
    }
    [[nodiscard]] const DogRepr& GetDogRepr() const {
        return dog_repr_;
    }
    [[nodiscard]] const GameSessionRepr& GetGameSessionRepr() const {
        return session_repr_;
    }
    [[nodiscard]] int GetIdCounter() const {
        return id_counter_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& name_;
        ar& id_;
        ar& dog_repr_;
        ar& session_repr_;
        ar& id_counter_;
    }

private:
    std::string name_;
    int id_;
    DogRepr dog_repr_;
    
    GameSessionRepr session_repr_;
    int id_counter_;
};

class PlayerReprTmp {
public:
    PlayerReprTmp() = default;

    explicit PlayerReprTmp(const model::Player& player, Token token)
        : name_(player.GetName())
        , id_(player.GetId())
        , dog_repr_(*player.GetDog())    
        , id_counter_(player.GetIdCounter())
        , token_(*token){
    }

    [[nodiscard]] std::string GetPlayerName() const {
        return name_;
    }
    [[nodiscard]] int GetPlayerId() const {
        return id_;
    }
    [[nodiscard]] const DogRepr& GetDogRepr() const {
        return dog_repr_;
    }
    [[nodiscard]] int GetIdCounter() const {
        return id_counter_;
    }
    [[nodiscard]] std::string GetPlayerToken() const {
        return token_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& name_;
        ar& id_;
        ar& dog_repr_;
        ar& id_counter_;
        ar& token_;
    }

private:
    std::string name_;
    int id_;
    DogRepr dog_repr_;
    int id_counter_;
    std::string token_;
};

class GameSessionReprTmp {
public:
    GameSessionReprTmp() = default;

    explicit GameSessionReprTmp(const model::GameSession& game_session, const PlayerTokens& player_tokens)
        : map_id_str_(*game_session.GetMap()->GetId()) {

        for(auto [token, player_ptr] : player_tokens.GetTokenToPlayerMap()) {
            PlayerReprTmp player_repr(*player_ptr, token);
            players_repr_.push_back(player_repr);
        }
        for(auto [id, loot_object_ptr] : game_session.GetLootObjects()) {
            LootObjectRepr loot_object_repr(*loot_object_ptr);
            loot_objects_repr_.push_back(loot_object_repr);
        }
    }

    [[nodiscard]] std::string GetMapIdString() const {
        return map_id_str_;
    }
    [[nodiscard]] const std::vector<PlayerReprTmp>& GetPlayerRepr() const {
        return players_repr_;
    }
    [[nodiscard]] const std::vector<LootObjectRepr>& GetLootsObjectRepr() const {
        return loot_objects_repr_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id_str_;
        ar& players_repr_;
        ar& loot_objects_repr_;
    }

private:
    std::string map_id_str_;
    std::vector<PlayerReprTmp> players_repr_;
    std::vector<LootObjectRepr> loot_objects_repr_;
};

void RestoreSession(model::Game& game, const GameSessionReprTmp& session_repr);
void Restore(model::Game& game);
void Save(model::Game& game);
void Restore(model::Game& game, std::string filename);
void Save(model::Game& game, std::string filename);

}



