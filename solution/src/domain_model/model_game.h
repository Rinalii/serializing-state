#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "model_env.h"

#include "loot_generator.h"
#include "collision_detector.h"

#include <unordered_map>

namespace model {
class Dog;
class Office;
class LootObject;
}

namespace model {
class Dog;

class LootObject : public collision_detector::Item {
public:
    LootObject(geom::Point2D position = {0., 0.}, double width = 0.) 
                : Item(position, width), id_(++id_counter_) {
    }

    LootObject(int type, geom::Point2D position = {0., 0.}, double width = 0.) 
                : Item(position, width), id_(++id_counter_), type_(type) {
    }

    LootObject(int type, int value, geom::Point2D position = {0., 0.}, double width = 0.) 
                : Item(position, width), id_(++id_counter_), type_(type), value_(value) {
    }

    void SetItem(const Item& item) {
        position = item.position;
        width = item.width;
    }
    void SetId(int id) {
        id_ = id;
    }
    
    void SetType(int type) {
        type_ = type;
    }
    void SetValue(int value) {
        value_ = value;
    }
    int GetValue() const {
        return value_;
    }
    int GetType() const {
        return type_;
    }
    int GetId() const {
        return id_;
    }
    geom::Point2D GetPosition() const {
        return position;
    }
    double GetWidth() const {
        return width;
    }
    int GetIdCounter() const {
        return id_counter_;
    }

    void SetIdCounter(int id_counter) {
        id_counter_ = id_counter;
    }

private:
    int id_;
    int type_ = 0;
    int value_ = 0;
    static int id_counter_;
};

class ItemGathererProviderImpl : public collision_detector::ItemGathererProvider {
public:
    size_t ItemsCount() const override{
        return items_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return *items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return *gatherers_.at(idx);
    }

    void AddItem(const std::shared_ptr<model::LootObject>& item) {
        items_.push_back(item);
    }

    void AddItem(const model::Office& item) {
        items_.push_back(std::make_shared<model::Office>(item));
    }

    void AddGatherer(const std::shared_ptr<model::Dog>& gatherer) {
        gatherers_.push_back(gatherer);
    }

    std::shared_ptr<model::Dog> GetDog(int idx) const {
        return std::dynamic_pointer_cast<model::Dog>(gatherers_.at(idx));
    }
    std::shared_ptr<model::Office> GetOffice(int idx) const {
        return std::dynamic_pointer_cast<model::Office>(items_.at(idx));
    }
    std::shared_ptr<model::LootObject> GetLootObject(int idx) const {
        return std::dynamic_pointer_cast<model::LootObject>(items_.at(idx));
    }
private:
    std::vector<std::shared_ptr<collision_detector::Item>> items_;
    std::vector<std::shared_ptr<collision_detector::Gatherer>> gatherers_;
};

class GameSession {
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

public:
    explicit GameSession(std::shared_ptr<Map> map) : map_(map) {}

    std::shared_ptr<Map> GetMap() const;
    void AddDog(std::shared_ptr<Dog> dog);
    const std::vector<std::shared_ptr<Dog>> GetDogs();
    void UpdateDogsPosition(double dt);

    const std::unordered_map<int, std::shared_ptr<LootObject>>& GetLootObjects() const{
        return loot_objects_;
    }
    int GetSizeLootObjects() const{
        return loot_objects_.size();
    }

    void GenerateLootObjects(int number) {
        for(int i = 0; i < number; ++i) {
            std::pair<int, int> type_and_value = map_->GetRandomTypeAndValueOfLoot();
            PointDouble pos = map_->GetRandomPosition();
            LootObject loot_object(type_and_value.first, type_and_value.second, geom::Point2D(pos.x, pos.y));
            loot_objects_[loot_object.GetId()] = std::make_shared<LootObject>(loot_object);
        }
    }

    void SetLootObjects(std::vector<LootObject>& loot_objects) {
        for(auto loot_object : loot_objects) {
            loot_objects_[loot_object.GetId()] = std::make_shared<LootObject>(loot_object);
        }
    }
    void AddLootObject(LootObject& loot_object) {
        loot_objects_[loot_object.GetId()] = std::make_shared<LootObject>(loot_object);
    }

    ItemGathererProviderImpl CreateProvider() {
        ItemGathererProviderImpl provider;
        for(const auto& id_to_item : loot_objects_) {
            provider.AddItem(id_to_item.second);
        }
        const std::vector<Office>& offices = map_->GetOffices();
        for(const auto& office : offices) {
            provider.AddItem(office);
        }
        const std::vector<std::shared_ptr<Dog>> dogs = GetDogs();
        for(const auto& dog : dogs) {
            provider.AddGatherer(dog);
        }
        return provider;
    }
    void CollectAndSendItems() {
        ItemGathererProviderImpl provider = CreateProvider();
        auto events = collision_detector::FindGatherEvents(provider);
        
        for(const auto& event : events) {
            int item_idx = event.item_id;
            std::shared_ptr<Dog> dog = provider.GetDog(event.gatherer_id);

            std::shared_ptr<Office> office = provider.GetOffice(item_idx);
            if(office) {
                dog->CleanBag();
                continue;
            }
            std::shared_ptr<LootObject> loot_object = provider.GetLootObject(item_idx);
            if(loot_object) {
                auto it_item = loot_objects_.find(loot_object->GetId());
                if(it_item != loot_objects_.end()) {
                    Bag& bag = dog->GetBag();
                    if(!bag.IsFull()) {
                        bag.AddLoot(it_item->second);
                        dog->AddScore(loot_object->GetValue());
                        loot_objects_.erase(it_item);
                    }
                }
            }
            

        }
        
    }

    const std::vector<std::shared_ptr<Dog>> GetDogs() const {
        std::vector<std::shared_ptr<Dog>> result;

        for (const auto& weak_dog_ptr : dogs_) {
            if (auto shared_dog_ptr = weak_dog_ptr.lock()) { 
                result.push_back(shared_dog_ptr);
            }
        }
        return result;
    }


private:
    std::shared_ptr<Map> map_;
    std::vector<std::weak_ptr<Dog>> dogs_;
    std::unordered_map<int, std::shared_ptr<LootObject>> loot_objects_;
};

}

