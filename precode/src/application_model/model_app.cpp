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

std::shared_ptr<Player> PlayerTokens::FindPlayer(const Token& token) const {
    auto it = token_to_player_.find(token);
    if(it != token_to_player_.end()) {
        return it->second;
    }
    return nullptr;
}
Token PlayerTokens::AddPlayer(const Player& player) {
    Token token = GetToken();
    token_to_player_[token] = std::make_shared<Player>(player);
    return token;
}

const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& PlayerTokens::GetTokenToPlayerMap() const {
    return token_to_player_;
}

Token PlayerTokens::GetToken() {
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << generator1_(); 
    ss << std::setw(16) << std::setfill('0') << std::hex << generator2_(); 
    std::string result = ss.str();
    assert(result.size() == 32);
    return Token(ss.str());
}
}


