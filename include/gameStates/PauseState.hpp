/* Copyright (c) 2025 Hammer Forged Games
 * All rights reserved.
 * Licensed under the MIT License - see LICENSE file for details
*/

#ifndef PAUSE_STATE_HPP
#define PAUSE_STATE_HPP

#include "gameStates/GameState.hpp"

class PauseState : public GameState {
 public:
  bool enter() override;
  void update() override;
  void render() override;
  bool exit() override;
  std::string getName() const override;
};

#endif  // PAUSE_STATE_HPP
