#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "tagged.h"


#include <random>

#include <memory>
#include <cmath>

#include <optional>

#include "collision_detector.h"

#include "loot.h"

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
    std::pair<PointDouble, PointDouble> GetArea() const noexcept;
    PointDouble GetRandomPosition() const noexcept;

private:
    Point start_;
    Point end_;

    static double GenerateRandomNumber(double min, double max);
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office : public collision_detector::Item {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : Item({static_cast<double>(position.x), static_cast<double>(position.y)}, model_constants::BASE_WIDTH)
        , id_{std::move(id)}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept;
    geom::Point2D GetPosition() const noexcept;
    double GetWidth() const;
    Offset GetOffset() const noexcept;

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

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
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

    PointDouble GetRandomPosition() const;

    void SetLootTypes(const std::vector<LootType>& loot_types);
    int GetNumberOfLootTypes() const;
    const std::vector<LootType>& GetLootTypes() const;
    int GetRandomTypeOfLoot() const;
    std::pair<int, int> GetRandomTypeAndValueOfLoot() const;

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

    std::vector<LootType> loot_types_;
    
    static int GenerateRandomNumber(int min, int max);
    static double GenerateRandomNumber(double min, double max);
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
    void CleanBag() {
        loot_objects.clear();
    }
};

class Dog : public collision_detector::Gatherer {
public:
    explicit Dog() : Gatherer(geom::Point2D{0., 0.}, geom::Point2D{0., 0.}, model_constants::DOG_WIDTH), id_(++id_counter_) {}
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
    void CleanBag() {
        bag_.CleanBag();
    }
    Bag& GetBag() {
        return bag_;
    }
    void SetPositionEndGatherer() {position_ = {end_pos.x, end_pos.y};}
    void AddScore(int value) {
        score += value;
    }
    int GetScore() const {
        return score;
    }
private:
    int id_;
    static int id_counter_;

    PointDouble position_;
    PointDouble speed_;
    Direction direction_=Direction::NORTH;

    double speed_value_ = 0.;

    Bag bag_;
    int score = 0;
};

}  // namespace model
