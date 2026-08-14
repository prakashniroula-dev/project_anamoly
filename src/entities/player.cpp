#include <entities/player.hpp>

void Player::update(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::update called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->update(win, dt);
}

void Player::draw(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::draw called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->draw(win, dt);
}

void Player::init()
{
  if (!player)
  {
    static Character defaultPlayer;
    player = &defaultPlayer;
  }
  player->init();
}