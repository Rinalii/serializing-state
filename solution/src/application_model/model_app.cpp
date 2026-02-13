#include "model_app.h"

#include <cassert>
#include <iomanip>

namespace model {

int Dog::id_counter_ = 0;
int Player::id_counter_ = 0;
int LootObject::id_counter_ = 0;

void Player::AddAndPrepareGameSession(std::shared_ptr<GameSession> session, std::shared_ptr<model::Map> map, bool is_rand_spawn) {
    session_ = session;

    dog_->ApplyMapSettings(map, is_rand_spawn);
    session_->AddDog(dog_);
}
}


