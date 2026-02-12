#include "model_env.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

Point PointDouble::Round() const {
    return Point{static_cast<Coord>(std::round(x)), static_cast<Coord>(std::round(y))};
}

PointDouble PointDouble::operator+(const PointDouble& other) const {
    return PointDouble{x+other.x, y+other.y};
}

bool PointDouble::operator<(const PointDouble& other) const {
    if(x<other.x && y<=other.y) {
        return true;
    }
    if(x<=other.x && y<other.y) {
        return true;
    }
    return false;
}

double PointDouble::Norm() const {
    return std::sqrt(x*x+y*y);
}

double PointDouble::Distance(const PointDouble& other) const {
    return std::sqrt((x-other.x)*(x-other.x)+(y-other.y)*(y-other.y));
}


Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

bool Road::IsOnArea(PointDouble position) const noexcept{
    bool is_hor = false;
    bool is_ver = false;
    double width = 0.4;

    Point start = start_;
    Point end = end_;

    if(IsHorizontal()) {
        if(start_.x > end.x) {
            start = end_;
            end = start_;
        }
        is_hor = (static_cast<double>(start.x)  - width <= position.x) && (position.x <= static_cast<double>(end.x) + width);
        is_ver = (static_cast<double>(start.y) - width <= position.y) && (position.y <= static_cast<double>(end.y) + width);
    } else {
        if(start_.y > end.y) {
            start = end_;
            end = start_;
        }
        is_ver = (static_cast<double>(start.y) - width <= position.y) && (position.y <= static_cast<double>(end.y) + width);
        is_hor = (static_cast<double>(start.x) - width <= position.x) && (position.x <= static_cast<double>(end.x) + width);
    }
    return is_hor && is_ver;
}

PointDouble Road::GetMaxPossiblePosition(PointDouble position) const noexcept {
    PointDouble max_possible = position;
    double width = 0.4;
    Point start = start_;
    Point end = end_;
    if(IsHorizontal()) {
        if(start_.x > end_.x) {
            start = end_;
            end = start_;
        }
        if(position.x < start.x - width) {
            max_possible.x = start.x - width;
        } else if(position.x > end.x+ width) {
            max_possible.x = end.x+ width;
        }
        if(position.y < start.y - width) {
            max_possible.y = start.y - width;
        } else if(position.y > start.y + width) {
            max_possible.y = start.y + width;
        }
    } else {
        if(start_.y > end_.y) {
            start = end_;
            end = start_;
        }
        if(position.x < start.x - width) {
            max_possible.x = start.x - width;
        } else if(position.x > start.x + width) {
            max_possible.x = start.x + width;
        }
        if(position.y < start.y - width) {
            max_possible.y = start.y - width;
        } else if(position.y > end.y+ width) {
            max_possible.y = end.y+ width;
        }
    }
    return max_possible;
}

std::pair<PointDouble, PointDouble> Road::GetArea() const noexcept {
    double width = 0.4;
    PointDouble min;
    PointDouble max;
    if(start_.x < end_.x) {
        min.x = start_.x - width;
        max.x = end_.x + width;
    } else {
        max.x = start_.x + width;
        min.x = end_.x - width;
    }
    if(start_.y < end_.y) {
        min.y = start_.y - width;
        max.y = end_.y + width;
    } else {
        max.y = start_.y + width;
        min.y = end_.y - width;
    }
    return {min, max};
}
PointDouble Road::GetRandomPosition() const noexcept {
    std::pair<PointDouble, PointDouble> area = GetArea();
    double x = GenerateRandomNumber(area.first.x, area.second.x);
    double y = GenerateRandomNumber(area.first.y, area.second.y);
    return PointDouble{x, y};
}

double Road::GenerateRandomNumber(double min, double max) {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    std::uniform_real_distribution<>distr(min, max);
    return distr(gen);
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

geom::Point2D Office::GetPosition() const noexcept {
    return position;
}

double Office::GetWidth() const {
    return width;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

void RoadIndexes::AddRoadIndexes(const std::vector<Road>& roads) {
    for(size_t i=0; i<roads.size(); ++i) {
        const Road& road = roads[i];
        if (road.IsHorizontal()) {
            coord_to_idx_hor_[road.GetStart().y] = i;
        } else {
            coord_to_idx_ver_[road.GetStart().x] = i;
        }
    }
}

std::vector<size_t> RoadIndexes::GetRoadIndexes(Point position) const {
    std::vector<size_t> res;
    auto it = coord_to_idx_hor_.find(position.y);
    if(it != coord_to_idx_hor_.end()) {
        res.push_back(it->second);
    }
    auto it2 = coord_to_idx_ver_.find(position.x);
    if(it2 != coord_to_idx_ver_.end()) {
        res.push_back(it2->second);
    }
    return res;
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

PointDouble Map::GetRandPosition() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    int rand_number_of_road = GenerateRandomNumber(0, roads_.size()-1);

    const Road& road = roads_[rand_number_of_road];
    Point road_start = road.GetStart();
    Point road_end = road.GetEnd();

    if (road.IsHorizontal()) {
        double random_x = GenerateRandomNumber(static_cast<double>(road_start.x), static_cast<double>(road_end.x));
        return PointDouble{random_x, static_cast<double>(road_start.y)};
    }
    double random_y = GenerateRandomNumber(static_cast<double>(road_start.y), static_cast<double>(road_end.y));
    return PointDouble{static_cast<double>(road_start.x), random_y};
}

PointDouble Map::GetStartPosition() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    const Road& road = roads_[0];
    Point road_start = road.GetStart();
    return PointDouble{road_start.x*1., road_start.y*1.};
}

void Map::SetDogSpeed(double dog_speed) {
    dog_speed_ = dog_speed;
}

double Map::GetDogSpeed() const {return dog_speed_;}

void Map::AddRoadIndexes() {
    coords_to_road_idx_.AddRoadIndexes(roads_);
}

std::vector<size_t> Map::GetRoadIndexes(Point position) const {
    return coords_to_road_idx_.GetRoadIndexes(position);
}

std::vector<Road> Map::GetRoadsByPosition(Point position) const {
    std::vector<Road> res;
    std::vector<size_t> roads_idxs = GetRoadIndexes(position);
    for(size_t idx : roads_idxs) {
        res.push_back(roads_[idx]);
    }
    return res;
}

PointDouble Map::GetRandomPosition() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    int rand_number_of_road = GenerateRandomNumber(0, roads_.size()-1);

    const Road& road = roads_[rand_number_of_road];
    return road.GetRandomPosition();
}

void Map::SetLootTypes(const std::vector<LootType>& loot_types) {
    loot_types_ = loot_types;
}

int Map::GetNumberOfLootTypes() const {
    return loot_types_.size();
}

const std::vector<LootType>& Map::GetLootTypes() const {
    return loot_types_;
}

int Map::GetRandomTypeOfLoot() const {
    return GenerateRandomNumber(0, GetNumberOfLootTypes()-1);
}

std::pair<int, int> Map::GetRandomTypeAndValueOfLoot() const {
    int type = GenerateRandomNumber(0, GetNumberOfLootTypes()-1);
    return {type, loot_types_[type].value};
}

int Map::GenerateRandomNumber(int min, int max) {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    std::uniform_int_distribution<>distr(min, max);
    return distr(gen);
}

double Map::GenerateRandomNumber(double min, double max) {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    std::uniform_real_distribution<>distr(min, max);
    return distr(gen);
}

void Dog::SetDirection(const std::string& direction_str) {
    if(direction_str == "U"){
        direction_ = Direction::NORTH;
        SetSpeed({0., -speed_value_});
    } else if(direction_str == "D"){
        direction_ = Direction::SOUTH;
        SetSpeed({0., speed_value_});
    } else if(direction_str == "R"){
        direction_ = Direction::WEST;
        SetSpeed({speed_value_, 0.});
    } else if(direction_str == "L"){
        direction_ = Direction::EAST;
        SetSpeed({-speed_value_, 0.});
    } else if(direction_str == ""){
        SetSpeed({0., 0.});
    } else {
        throw;
    }
}

std::string Dog::GetDirection() const {
    switch(direction_){
        case Direction::NORTH: return "U";
        case Direction::SOUTH: return "D";
        case Direction::WEST: return "R";
        case Direction::EAST: return "L";
    }
    return "U";
}
void Dog::SetSpeedValue(double speed_value) {speed_value_ = speed_value;}
void Dog::ApplyMapSettings(std::shared_ptr<model::Map> map, bool is_rand_spawn) {
    if(is_rand_spawn) {
        SetPosition(map->GetRandPosition());
    } else {
        SetPosition(map->GetStartPosition());
    }
    SetSpeedValue(map->GetDogSpeed());
    Stop();
}
void Dog::Stop() {
    SetSpeed({0, 0});
}

}  // namespace model
