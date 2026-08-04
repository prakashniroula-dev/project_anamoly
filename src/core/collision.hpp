
#pragma once
#include <SFML/Graphics.hpp>

namespace Collision
{
  struct Result
  {
    bool collided;
    sf::Vector2f overlap;
  };

  Result getCollision(const sf::FloatRect &a, const sf::FloatRect &b)
  {
    Result result;
    auto intersect = a.findIntersection(b);
    if (intersect.has_value())
    {
      result.collided = true;
      result.overlap = intersect->size; // SFML 3.x uses .size
    }
    else
    {
      result.collided = false;
      result.overlap = sf::Vector2f(0.f, 0.f);
    }
    return result;
  }

  void resolveCollision(sf::FloatRect &a, const sf::FloatRect &b, sf::Vector2f &pos)
  {
    auto result = getCollision(a, b);
    if (!result.collided)
      return;

    // Resolve on the axis with the smaller overlap
    if (result.overlap.x < result.overlap.y)
    {
      // Use centre comparison for correct direction
      float sign = (a.position.x + a.size.x / 2.f < b.position.x + b.size.x / 2.f) ? 1.f : -1.f;
      pos.x -= result.overlap.x * sign;
    }
    else
    {
      float sign = (a.position.y + a.size.y / 2.f < b.position.y + b.size.y / 2.f) ? 1.f : -1.f;
      pos.y -= result.overlap.y * sign;
    }
    a.position = pos; // keep rect in sync
  }
}