#pragma once

#include <optional>
#include <string>

namespace model_constants{
    const double LOOT_WIDTH = 0.;
    const double DOG_WIDTH = 0.6;
    const double BASE_WIDTH = 0.5;
}

namespace model {

struct LootType {
    std::string name;
    std::string file;
    std::string type;
    std::optional<int> rotation;
    std::string color;
    std::optional<double> scale;
    int value = 0;
};

class LootObject : public collision_detector::Item {
public:
    LootObject(geom::Point2D position = {0., 0.}, double width = model_constants::LOOT_WIDTH) 
                : Item(position, width), id_(++id_counter_) {
    }

    LootObject(int type, geom::Point2D position = {0., 0.}, double width = 0.) 
                : Item(position, width), id_(++id_counter_), type_(type) {
    }

    LootObject(int type, int value, geom::Point2D position = {0., 0.}, double width = 0.) 
                : Item(position, width), id_(++id_counter_), type_(type), value_(value) {
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

private:
    int id_;
    int type_ = 0;
    int value_ = 0;
    static int id_counter_;
};

}