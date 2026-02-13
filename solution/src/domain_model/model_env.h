#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"


#include <random>

#include <memory>
#include <cmath>

#include <optional>

#include <boost/json.hpp>
#include "collision_detector.h"


namespace model_constants{
    const std::string X = "x";
    const std::string Y = "y";
    const std::string H = "h";
    const std::string W = "w";
    const std::string X0 = "x0";
    const std::string Y0 = "y0";
    const std::string X1 = "x1";
    const std::string Y1 = "y1";
    const std::string OFFSET_X = "offsetX";
    const std::string OFFSET_Y = "offsetY";
}

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct PointDouble {
    double x = 0;
    double y = 0;

    Point Round() const;
    PointDouble operator+(const PointDouble& other) const;

    template <typename T>
    PointDouble operator*(T a) {
        return PointDouble{x*a, y*a};
    }

    bool operator<(const PointDouble& other) const;
    double Norm() const;
    double Distance(const PointDouble& other) const;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

    bool IsOnArea(PointDouble position) const noexcept;
    PointDouble GetMaxPossiblePosition(PointDouble position) const noexcept;
    std::pair<PointDouble, PointDouble> GetArea() const noexcept {
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
    PointDouble GetRandomPosition() const noexcept {
        std::pair<PointDouble, PointDouble> area = GetArea();
        double x = GenerateRandomNumber(area.first.x, area.second.x);
        double y = GenerateRandomNumber(area.first.y, area.second.y);
        return PointDouble{x, y};
    }

private:
    Point start_;
    Point end_;

    static double GenerateRandomNumber(double min, double max) {
        static std::random_device random_device;
        static std::mt19937 gen(random_device());
        std::uniform_real_distribution<>distr(min, max);
        return distr(gen);
    }
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office : public collision_detector::Item {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : Item({static_cast<double>(position.x), static_cast<double>(position.y)}, 0.5)
        , id_{std::move(id)}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    geom::Point2D GetPosition() const noexcept {
        return position;
    }

    double GetWidth() const {
        return width;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Offset offset_;
};


class RoadIndexes {
public:
    RoadIndexes() {}
    void AddRoadIndexes(const std::vector<Road>& roads);
    std::vector<size_t> GetRoadIndexes(Point position) const;

private:
    std::unordered_map<Coord, size_t> coord_to_idx_hor_;
    std::unordered_map<Coord, size_t> coord_to_idx_ver_;
};

struct LootType {
    std::string name;
    std::string file;
    std::string type;
    std::optional<int> rotation;
    std::string color;
    std::optional<double> scale;
    int value = 0;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    PointDouble GetRandPosition() const;
    PointDouble GetStartPosition() const;

    void SetDogSpeed(double dog_speed);
    void SetBagCapacity(int bag_capacity = 3) {bag_capacity_ = bag_capacity;}
    double GetDogSpeed() const;
    int GetBagCapacity() const {return bag_capacity_;}
    void AddRoadIndexes();

    std::vector<size_t> GetRoadIndexes(Point position) const;
    std::vector<Road> GetRoadsByPosition(Point position) const;

    PointDouble GetRandomPosition() const {
        if (roads_.empty()) {
            throw std::runtime_error("No roads on the map");
        }

        int rand_number_of_road = GenerateRandomNumber(0, roads_.size()-1);

        const Road& road = roads_[rand_number_of_road];
        return road.GetRandomPosition();
    }

    void SetLootTypesJson(const boost::json::array& loot_types) {
        loot_types_json_ = loot_types;
    }

    void SetLootTypes(const std::vector<LootType>& loot_types) {
        loot_types_ = loot_types;
    }

    int GetNumberOfLootTypes() const {
        return loot_types_.size();
    }

    const boost::json::array& GetLootTypesJson() {
        return loot_types_json_;
    }
    const std::vector<LootType>& GetLootTypes() const {
        return loot_types_;
    }

    int GetRandomTypeOfLoot() const {
        return GenerateRandomNumber(0, GetNumberOfLootTypes()-1);
    }

    std::pair<int, int> GetRandomTypeAndValueOfLoot() const {
        int type = GenerateRandomNumber(0, GetNumberOfLootTypes()-1);
        return {type, loot_types_[type].value};
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_;
    int bag_capacity_ = 3;


    RoadIndexes coords_to_road_idx_;

    boost::json::array loot_types_json_;
    std::vector<LootType> loot_types_;
    

    static int GenerateRandomNumber(int min, int max) {
        static std::random_device random_device;
        static std::mt19937 gen(random_device());
        std::uniform_int_distribution<>distr(min, max);
        return distr(gen);
    }

    static double GenerateRandomNumber(double min, double max) {
        static std::random_device random_device;
        static std::mt19937 gen(random_device());
        std::uniform_real_distribution<>distr(min, max);
        return distr(gen);
    }
};

enum class Direction{
    NORTH, 
    SOUTH, 
    WEST, 
    EAST
};

class LootObject;

struct Bag {
    int capacity = 3;
    std::vector<std::shared_ptr<LootObject>> loot_objects;

    bool IsFull() const {
        return !(loot_objects.size() < capacity);
    }
    void AddLoot(const std::shared_ptr<LootObject>& item) {
        loot_objects.push_back(item);
    }
};

class Dog : public collision_detector::Gatherer {
public:
    explicit Dog(int player_id) : Gatherer(geom::Point2D{0., 0.}, geom::Point2D{0., 0.}, 0.6), id_(++id_counter_), player_id_(player_id){}
    ~Dog() {}

    int GetId() const {return id_;}
    void SetPosition(PointDouble position) {position_ = position;}
    void SetSpeed(PointDouble speed) {speed_ = speed;}
    void SetDirection(Direction direction) {direction_ = direction;}
    void SetDirection(const std::string& direction_str);

    PointDouble GetPosition() const {return position_;}
    PointDouble GetSpeed() const {return speed_;}
    std::string GetDirection() const;
    void SetSpeedValue(double speed_value);
    void ApplyMapSettings(std::shared_ptr<model::Map> map, bool is_rand_spawn);
    void Stop();

    void SetGatherer(geom::Point2D curr_pos, geom::Point2D next_pos) {
        start_pos = curr_pos;
        end_pos = next_pos;
    }
    void SetGatherer(geom::Point2D next_pos) {
        start_pos = end_pos;
        end_pos = next_pos;
    }
    void SetWidth(double width) {
        width = width;
    }
    void CleanBag() {
        bag_.loot_objects.clear();
    }
    Bag& GetBag() {
        return bag_;
    }
    const Bag& GetBag() const {
        return bag_;
    }
    void SetPositionEndGatherer() {position_ = {end_pos.x, end_pos.y};}
    void AddScore(int value) {
        score += value;
    }
    int GetScore() const {
        return score;
    }
    int GetIdCounter() const{
        return id_counter_;
    }
    void SetIdCounter(int id_counter) const{
        id_counter_ = id_counter;
    }
    double GetSpeedValue() const{
        return speed_value_;
    }
    void SetId(int id) {
        id_ = id;
    }
    void SetBag(const Bag& bag) {
        bag_ = bag;
    }

    Direction GetDirectionEnum() const {
        return direction_;
    }
private:
    int id_;
    int player_id_;
    static int id_counter_;

    PointDouble position_;
    PointDouble speed_;
    Direction direction_=Direction::NORTH;

    double speed_value_ = 0.;

    Bag bag_;
    int score = 0;
};

}  // namespace model
