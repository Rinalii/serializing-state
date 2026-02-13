#include <cmath>

#include "../src/serialization/model_serialization.h"

#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/domain_model/collision_detector.h"
#include "../src/domain_model/model_game.h"

#include <boost/serialization/export.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>

using namespace geom;
using Catch::Matchers::WithinRel;


namespace {
    const std::string TAG = "[Serialization]";
}

using namespace model;

using namespace collision_detector;
TEST_CASE("ITEM SERIALIZATION", TAG) {
    Item item1{{10, 5}, 6};
    Item item2{{1, 2}, 3};

    std::stringstream ss;

    // Сериализация
    {
        boost::archive::text_oarchive oa{ss};
        oa << item1;
        oa << item2;
    }

    // Десериализация
    Item loaded_item1;
    Item loaded_item2;
    {
        boost::archive::text_iarchive ia{ss};
        ia >> loaded_item1;
        ia >> loaded_item2;
    }

    // Проверка
    std::cout << "Original item x: " << item1.position.x << ", item y: " << item1.position.y << ",  item width: " << item1.width << std::endl;
    std::cout << "Loaded item x: " << loaded_item1.position.x << ", loaded_item y: " << loaded_item1.position.y << ",  loaded_item width: " << loaded_item1.width << std::endl;
    
    CHECK(item1.position.x == loaded_item1.position.x);
    CHECK(item1.position.y == loaded_item1.position.y);
    CHECK(item1.width == loaded_item1.width);

    CHECK(item2.position.x == loaded_item2.position.x);
    CHECK(item2.position.y == loaded_item2.position.y);
    CHECK(item2.width == loaded_item2.width);
}

TEST_CASE("LOOTOBJECT SERIALIZATION", TAG) {
    Item item1{{10, 5}, 6};
    Item item2{{1, 2}, 3};

    LootObject loot_object1(12, 13, item1.position, item1.width); 
    LootObject loot_object2(14, 15, item2.position, item2.width); 
    LootObjectRepr loot_object_repr1(loot_object1);
    LootObjectRepr loot_object_repr2(loot_object2);

    // 2. Преобразование в DogRepr

    const std::string filename = "loot_object_archive.txt";

    // --- ЗАПИСЬ В АРХИВ (СЕРИАЛИЗАЦИЯ) ---
    try {
        std::ofstream ofs(filename);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }
        boost::archive::text_oarchive oa(ofs);
        oa << loot_object_repr1; // Запись объекта в архив
        oa << loot_object_repr2;
        ofs.close();
        std::cout << "\nDogRepr successfully saved to " << filename << std::endl;
    } catch (const boost::archive::archive_exception& e) {
        std::cerr << "Boost Archive Exception during save: " << e.what() << std::endl;
        return;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception during save: " << e.what() << std::endl;
        return;
    }

    LootObject loaded_loot_object1;
    LootObject loaded_loot_object2;
    // --- ЧТЕНИЕ ИЗ АРХИВА (ДЕСЕРИАЛИЗАЦИЯ) ---
    LootObjectRepr loaded_loot_object_repr1; // Создаем пустой объект для загрузки данных
    LootObjectRepr loaded_loot_object_repr2;
    try {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + filename);
        }
        boost::archive::text_iarchive ia(ifs);
        ia >> loaded_loot_object_repr1; // Чтение объекта из архива
        ia >> loaded_loot_object_repr2;
        ifs.close();

        // 3. Восстановление в model::Dog
        loaded_loot_object1 = loaded_loot_object_repr1.Restore();
        loaded_loot_object2 = loaded_loot_object_repr2.Restore();

    } catch (const boost::archive::archive_exception& e) {
        std::cerr << "Boost Archive Exception during load: " << e.what() << std::endl;
        return;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception during load: " << e.what() << std::endl;
        return;
    }
    
    CHECK(loot_object1.GetPosition().x == loaded_loot_object1.GetPosition().x);
    CHECK(loot_object1.GetPosition().y == loaded_loot_object1.GetPosition().y);
    CHECK(loot_object1.GetType() == loaded_loot_object1.GetType());
    CHECK(loot_object1.GetValue() == loaded_loot_object1.GetValue());
    CHECK(loot_object1.GetId() == loaded_loot_object1.GetId());
    CHECK(loot_object1.GetWidth() == loaded_loot_object1.GetWidth());
    CHECK(loot_object1.GetIdCounter() == loaded_loot_object1.GetIdCounter());


    CHECK(loot_object2.GetPosition().x == loaded_loot_object2.GetPosition().x);
    CHECK(loot_object2.GetPosition().y == loaded_loot_object2.GetPosition().y);
    CHECK(loot_object2.GetType() == loaded_loot_object2.GetType());
    CHECK(loot_object2.GetValue() == loaded_loot_object2.GetValue());
    CHECK(loot_object2.GetId() == loaded_loot_object2.GetId());
    CHECK(loot_object2.GetWidth() == loaded_loot_object2.GetWidth());
    CHECK(loot_object2.GetIdCounter() == loaded_loot_object2.GetIdCounter());
}

TEST_CASE("BAG SERIALIZATION", TAG) {
    Item item1{{10, 5}, 6};
    Item item2{{1, 2}, 3};

    Bag bag;
    std::shared_ptr<LootObject> loot_object_ptr1 = std::make_shared<LootObject>(12, 13, item1.position, item1.width); 
    std::shared_ptr<LootObject> loot_object_ptr2 = std::make_shared<LootObject>(22, 23, item2.position, item2.width); 
    bag.AddLoot(loot_object_ptr1);
    bag.AddLoot(loot_object_ptr2);

    BagRepr bag_repr(bag);

    std::stringstream ss;
    // Сериализация
    {
        boost::archive::text_oarchive oa{ss};
        oa << bag_repr;
    }

    // Десериализация
    Bag loaded_bag;
    {
        boost::archive::text_iarchive ia{ss};
        BagRepr loaded_bag_repr;
        ia >> loaded_bag_repr;
        loaded_bag = loaded_bag_repr.Restore();
    }
 
    CHECK(bag.capacity == loaded_bag.capacity);
    REQUIRE(loaded_bag.loot_objects.size() == 2);
    CHECK(bag.loot_objects[0]->GetId() == loaded_bag.loot_objects[0]->GetId());
    CHECK(bag.loot_objects[0]->GetValue() == loaded_bag.loot_objects[0]->GetValue());
    CHECK(bag.loot_objects[0]->GetType() == loaded_bag.loot_objects[0]->GetType());
    CHECK(bag.loot_objects[0]->GetIdCounter() == loaded_bag.loot_objects[0]->GetIdCounter());
    CHECK(bag.loot_objects[0]->GetPosition().x == loaded_bag.loot_objects[0]->GetPosition().x);
    CHECK(bag.loot_objects[0]->GetPosition().y == loaded_bag.loot_objects[0]->GetPosition().y);
    CHECK(bag.loot_objects[0]->GetWidth() == loaded_bag.loot_objects[0]->GetWidth());

    CHECK(bag.loot_objects[1]->GetId() == loaded_bag.loot_objects[1]->GetId());
    CHECK(bag.loot_objects[1]->GetValue() == loaded_bag.loot_objects[1]->GetValue());
    CHECK(bag.loot_objects[1]->GetType() == loaded_bag.loot_objects[1]->GetType());
    CHECK(bag.loot_objects[1]->GetIdCounter() == loaded_bag.loot_objects[1]->GetIdCounter());
    CHECK(bag.loot_objects[1]->GetPosition().x == loaded_bag.loot_objects[1]->GetPosition().x);
    CHECK(bag.loot_objects[1]->GetPosition().y == loaded_bag.loot_objects[1]->GetPosition().y);
    CHECK(bag.loot_objects[1]->GetWidth() == loaded_bag.loot_objects[1]->GetWidth());
}


TEST_CASE("DOG SERIALIZATION", TAG) {
    Item item1{{10, 5}, 6};
    Item item2{{1, 2}, 3};

    Bag bag;
    std::shared_ptr<LootObject> loot_object_ptr1 = std::make_shared<LootObject>(12, 13, item1.position, item1.width); 
    std::shared_ptr<LootObject> loot_object_ptr2 = std::make_shared<LootObject>(22, 23, item2.position, item2.width); 
    bag.AddLoot(loot_object_ptr1);
    bag.AddLoot(loot_object_ptr2);

    model::Dog dog(123);
    dog.SetId(123);
    dog.SetPosition({1., 2.});
    dog.SetSpeedValue(5.);
    dog.SetSpeed({6., 7.});
    dog.SetDirection(Direction::SOUTH);
    dog.SetGatherer({0., 9.}, {7., 10.});
    dog.SetWidth(0.55);
    dog.SetBag(bag);

    DogRepr dog_repr(dog);

    std::stringstream ss;
    // Сериализация
    {
        boost::archive::text_oarchive oa{ss};
        oa << dog_repr;
    }

    // Десериализация
    Dog loaded_dog(234);
    {
        boost::archive::text_iarchive ia{ss};
        DogRepr loaded_dog_repr;
        ia >> loaded_dog_repr;
        loaded_dog = loaded_dog_repr.Restore();
    }

    bag = dog.GetBag();
    Bag loaded_bag = loaded_dog.GetBag();
 
    CHECK(bag.capacity == loaded_bag.capacity);
    REQUIRE(loaded_bag.loot_objects.size() == 2);
    CHECK(bag.loot_objects[0]->GetId() == loaded_bag.loot_objects[0]->GetId());
    CHECK(bag.loot_objects[0]->GetValue() == loaded_bag.loot_objects[0]->GetValue());
    CHECK(bag.loot_objects[0]->GetType() == loaded_bag.loot_objects[0]->GetType());
    CHECK(bag.loot_objects[0]->GetIdCounter() == loaded_bag.loot_objects[0]->GetIdCounter());
    CHECK(bag.loot_objects[0]->GetPosition().x == loaded_bag.loot_objects[0]->GetPosition().x);
    CHECK(bag.loot_objects[0]->GetPosition().y == loaded_bag.loot_objects[0]->GetPosition().y);
    CHECK(bag.loot_objects[0]->GetWidth() == loaded_bag.loot_objects[0]->GetWidth());

    CHECK(bag.loot_objects[1]->GetId() == loaded_bag.loot_objects[1]->GetId());
    CHECK(bag.loot_objects[1]->GetValue() == loaded_bag.loot_objects[1]->GetValue());
    CHECK(bag.loot_objects[1]->GetType() == loaded_bag.loot_objects[1]->GetType());
    CHECK(bag.loot_objects[1]->GetIdCounter() == loaded_bag.loot_objects[1]->GetIdCounter());
    CHECK(bag.loot_objects[1]->GetPosition().x == loaded_bag.loot_objects[1]->GetPosition().x);
    CHECK(bag.loot_objects[1]->GetPosition().y == loaded_bag.loot_objects[1]->GetPosition().y);
    CHECK(bag.loot_objects[1]->GetWidth() == loaded_bag.loot_objects[1]->GetWidth());

    CHECK(dog.GetId() == loaded_dog.GetId());
    CHECK(dog.GetPosition().x == loaded_dog.GetPosition().x);
    CHECK(dog.GetPosition().y == loaded_dog.GetPosition().y);
    CHECK(dog.GetSpeedValue() == loaded_dog.GetSpeedValue());
    CHECK(dog.GetSpeed().x == loaded_dog.GetSpeed().x);
    CHECK(dog.GetSpeed().y == loaded_dog.GetSpeed().y);
    CHECK(dog.GetDirectionEnum() == loaded_dog.GetDirectionEnum());
    CHECK(dog.start_pos.x == loaded_dog.start_pos.x);
    CHECK(dog.start_pos.y == loaded_dog.start_pos.y);
    CHECK(dog.end_pos.x == loaded_dog.end_pos.x);
    CHECK(dog.end_pos.y == loaded_dog.end_pos.y);
    CHECK(dog.width == loaded_dog.width);

}
