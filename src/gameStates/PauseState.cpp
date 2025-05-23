/* Copyright (c) 2025 Hammer Forged Games
 * All rights reserved.
 * Licensed under the MIT License - see LICENSE file for details
*/

#include "gameStates/PauseState.hpp"
#include "managers/InputManager.hpp"
#include "managers/FontManager.hpp"
#include "core/GameEngine.hpp"
#include <iostream>

bool PauseState::enter() {
  std::cout << "Forge Game Engine - Entering PAUSE State\n";
  return true;
}

void PauseState::update() {
  //std::cout << "Updating PAUSE State\n";
  // Handle pause and ESC key.
  if (InputManager::Instance().isKeyDown(SDL_SCANCODE_R)) {
      // Flag the GamePlayState transition
      // We'll do the actual removal in GamePlayState::enter()
      GameEngine::Instance().getGameStateManager()->setState("GamePlayState");
  }
  if (InputManager::Instance().isKeyDown(SDL_SCANCODE_ESCAPE)) {
      GameEngine::Instance().setRunning(false);
  }
}

void PauseState::render() {
    SDL_Color fontColor = {200, 200, 200, 255};//gray
     FontManager::Instance().drawText(
       "Pause State Place Holder <----> Press R to Return to test Player",
       "fonts_Arial",
       GameEngine::Instance().getWindowWidth() / 2,     // Center horizontally
       (GameEngine::Instance().getWindowHeight() / 2) - 180,
       fontColor,
       GameEngine::Instance().getRenderer());
}
bool PauseState::exit() {
  std::cout << "Forge Game Engine - Exiting PAUSE State\n";

  return true;
}

std::string PauseState::getName() const {
  return "PauseState";
}
