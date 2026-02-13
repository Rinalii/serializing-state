#include "model_game.h"

#include <iostream>

using namespace std::literals;

namespace model {

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

}