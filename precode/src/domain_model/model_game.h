#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "model_env.h"

#include "loot_generator.h"
#include "collision_detector.h"

#include "loot.h"

namespace model {

class ItemGathererProviderImpl : public collision_detector::ItemGathererProvider {
public:
    size_t ItemsCount() const override;
    collision_detector::Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    collision_detector::Gatherer GetGatherer(size_t idx) const override;

    void AddItem(const std::shared_ptr<model::LootObject>& item);
    void AddItem(const model::Office& item);
    void AddGatherer(const std::shared_ptr<model::Dog>& gatherer);

    std::shared_ptr<model::Dog> GetDog(int idx) const;
    std::shared_ptr<model::Office> GetOffice(int idx) const;
    std::shared_ptr<model::LootObject> GetLootObject(int idx) const;

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

    const std::unordered_map<int, std::shared_ptr<LootObject>>& GetLootObjects() const;
    int GetSizeLootObjects() const;
    void GenerateLootObjects(int number);

private:
    std::shared_ptr<Map> map_;
    std::vector<std::weak_ptr<Dog>> dogs_;

    std::unordered_map<int, std::shared_ptr<LootObject>> loot_objects_;

    ItemGathererProviderImpl CreateProvider();
    void CollectAndSendItems();
};

class Game {
public:
    using Maps = std::vector<Map>;

    Game(double base_interval = 0.5, double probability = 0.5)
        : loot_generator_(
            std::chrono::milliseconds(static_cast<long long>(base_interval * 1000)),
            probability){
    }

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    std::shared_ptr<GameSession> GetGameSession(const Map::Id& id);
    std::shared_ptr<GameSession> GetGameSession(std::shared_ptr<Map> map);
    void PrintMaps() const;
    void UpdateGame(double dt);

    void GenerateLoot(double time_delta_sec);

private:
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, util::TaggedHasher<Map::Id>>;

    MapIdToIndex map_id_to_index_;
    std::vector<Map> maps_;

    std::vector<std::shared_ptr<GameSession>> game_sessions_;

    loot_gen::LootGenerator loot_generator_;
};

}

