#include "model_game.h"

#include <iostream>

using namespace std::literals;

namespace model {

size_t ItemGathererProviderImpl::ItemsCount() const {
    return items_.size();
}

collision_detector::Item ItemGathererProviderImpl::GetItem(size_t idx) const {
    return *items_.at(idx);
}

size_t ItemGathererProviderImpl::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer ItemGathererProviderImpl::GetGatherer(size_t idx) const {
    return *gatherers_.at(idx);
}

void ItemGathererProviderImpl::AddItem(const std::shared_ptr<model::LootObject>& item) {
    items_.push_back(item);
}

void ItemGathererProviderImpl::AddItem(const model::Office& item) {
    items_.push_back(std::make_shared<model::Office>(item));
}

void ItemGathererProviderImpl::AddGatherer(const std::shared_ptr<model::Dog>& gatherer) {
    gatherers_.push_back(gatherer);
}

std::shared_ptr<model::Dog> ItemGathererProviderImpl::GetDog(int idx) const {
    return std::dynamic_pointer_cast<model::Dog>(gatherers_.at(idx));
}
std::shared_ptr<model::Office> ItemGathererProviderImpl::GetOffice(int idx) const {
    return std::dynamic_pointer_cast<model::Office>(items_.at(idx));
}
std::shared_ptr<model::LootObject> ItemGathererProviderImpl::GetLootObject(int idx) const {
    return std::dynamic_pointer_cast<model::LootObject>(items_.at(idx));
}



std::shared_ptr<Map> GameSession::GetMap() const {
    return map_;
}

void GameSession::AddDog(std::shared_ptr<Dog> dog) {
    dog->GetBag().capacity = map_->GetBagCapacity();
    dogs_.emplace_back(dog);
}

const std::vector<std::shared_ptr<Dog>> GameSession::GetDogs(){
    std::vector<std::shared_ptr<Dog>> result;
    result.reserve(dogs_.size());

    auto it = dogs_.begin();
    while (it != dogs_.end()) {
        if (it->expired()) {
            it = dogs_.erase(it);
        } else {
            result.push_back(it->lock());
            ++it;
        }
    }
    return result;
}

void GameSession::UpdateDogsPosition(double dt) {
    const auto& map = GetMap();
    for (auto dog : GetDogs()) {
        PointDouble curr_pos = dog->GetPosition();
        Point curr_pos_int = curr_pos.Round();

        std::vector<Road> roads_at_point = map->GetRoadsByPosition(curr_pos_int);

        PointDouble speed = dog->GetSpeed();
        PointDouble next_pos = curr_pos + PointDouble{speed.x * dt, speed.y * dt};

        PointDouble max_possible_pos = curr_pos;

        bool is_stop = true;

        for(const auto& road : roads_at_point) {
            if(road.IsOnArea(next_pos)) {
                is_stop = false;
                max_possible_pos = next_pos;
                break;
            } else {
                PointDouble tmp_max_possible = road.GetMaxPossiblePosition(next_pos);
                double dist = curr_pos.Distance(max_possible_pos);
                double tmp_dist = curr_pos.Distance(tmp_max_possible);
                if(dist < tmp_dist) {
                    max_possible_pos = tmp_max_possible;
                }
            }
        }
        if(!is_stop) {
            dog->SetGatherer({next_pos.x, next_pos.y});
            dog->SetPositionEndGatherer();
        } else {
            dog->SetGatherer({max_possible_pos.x, max_possible_pos.y});
            dog->SetPositionEndGatherer();
            dog->Stop();
        }
    }
    CollectAndSendItems();
}

const std::unordered_map<int, std::shared_ptr<LootObject>>& GameSession::GetLootObjects() const{
    return loot_objects_;
}
int GameSession::GetSizeLootObjects() const{
    return loot_objects_.size();
}

void GameSession::GenerateLootObjects(int number) {
    for(int i = 0; i < number; ++i) {
        std::pair<int, int> type_and_value = map_->GetRandomTypeAndValueOfLoot();
        PointDouble pos = map_->GetRandomPosition();
        LootObject loot_object(type_and_value.first, type_and_value.second, geom::Point2D(pos.x, pos.y));
        loot_objects_[loot_object.GetId()] = std::make_shared<LootObject>(loot_object);
    }
}

ItemGathererProviderImpl GameSession::CreateProvider() {
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

void GameSession::CollectAndSendItems() {
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


void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept  {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return std::make_shared<Map>(maps_.at(it->second));
    }
    return nullptr;
}

std::shared_ptr<GameSession> Game::GetGameSession(const Map::Id& id) {
    auto map = FindMap(id);
    return GetGameSession(map);
}

std::shared_ptr<GameSession> Game::GetGameSession(std::shared_ptr<Map> map) {
    if (map == nullptr) {
        throw std::invalid_argument("Map doesn't exist");
    }
    const auto& id = map->GetId();

    auto it = std::find_if(game_sessions_.begin(), game_sessions_.end(),
                            [&id](const std::shared_ptr<GameSession>& session) {
                                return session->GetMap()->GetId() == id;
                            });
    
    if (it != game_sessions_.end()) {
        return *it;
    }

    auto game_session = std::make_shared<GameSession>(map);
    game_sessions_.push_back(game_session);

    return game_session;
}

void Game::PrintMaps() const {
    for (Map map : maps_) {
        std::cout << "Map: " << map.GetName() << std::endl;
        for (auto road : map.GetRoads()) {
            std::cout << "{" << road.GetStart().x << ", " << road.GetStart().y << "} - {" << road.GetEnd().x << ", " << road.GetEnd().y << "}" << std::endl;
        }
        std::cout << std::endl;
    }
}

void Game::UpdateGame(double dt) {
    for(auto& game_session : game_sessions_) {
        game_session->UpdateDogsPosition(dt);
    }
}

void Game::GenerateLoot(double time_delta_sec) {
    
    for(auto& session : game_sessions_) {
        const std::vector<std::shared_ptr<Dog>> dogs = session->GetDogs();
        int loot_count = session->GetSizeLootObjects();
        unsigned looter_count = dogs.size();
        unsigned number = loot_generator_.Generate(std::chrono::milliseconds(static_cast<long long>(time_delta_sec * 1000))
                        , loot_count, looter_count);
        session->GenerateLootObjects(number);
    }
}

}