/* Copyright (c) 2025 Hammer Forged Games
 * All rights reserved.
 * Licensed under the MIT License - see LICENSE file for details
*/

#include "managers/InputManager.hpp"
#include "core/GameEngine.hpp"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_joystick.h"
#include "utils/Vector2D.hpp"
#include <iostream>
#include <memory>

// Global pointer to store gamepad IDs
SDL_JoystickID* g_gamepadIDs{nullptr};

InputManager::InputManager()
    : m_keystates(nullptr),
      m_mousePosition(std::make_unique<Vector2D>(0, 0)) {
  // Create button states for the mouse
  for (int i = 0; i < 3; i++) {
    m_mouseButtonStates.push_back(false);
  }
}

InputManager::~InputManager() {
  // No need to delete m_mousePosition - smart pointer handles it

  // Clear all vectors
  //m_joystickValues.clear();
  //m_joysticks.clear();
  m_buttonStates.clear();
  m_mouseButtonStates.clear();
}

void InputManager::initializeGamePad() {
  // Check if gamepad subsystem is already initialized
  if (m_gamePadInitialized) {
    return;
  }

  // Initialize gamepad subsystem
  if (!SDL_Init(SDL_INIT_GAMEPAD)) {
    std::cerr << "Forge Game Engine - Failed to initialize gamepad subsystem: "
              << SDL_GetError() << std::endl;
    return;
  }

  // Get all available gamepads
  int numGamepads = 0;
  g_gamepadIDs = SDL_GetGamepads(&numGamepads);

  if (g_gamepadIDs == nullptr) {
    std::cerr << "Forge Game Engine - Failed to get gamepad IDs: "
              << SDL_GetError() << std::endl;
    return;
  }

  if (numGamepads > 0) {
    std::cout << "Forge Game Engine - Number of Game Pads detected: " << numGamepads << std::endl;
    // Open all available gamepads
    for (int i = 0; i < numGamepads; i++) {
      if (SDL_IsGamepad(g_gamepadIDs[i])) {
        SDL_Gamepad* gamepad = SDL_OpenGamepad(g_gamepadIDs[i]);
        if (gamepad) {
          m_joysticks.push_back(gamepad);
          std::cout << "Forge Game Engine - Gamepad connected: " << SDL_GetGamepadName(gamepad) << std::endl;

          // Add default joystick values
          m_joystickValues.push_back(std::make_pair(std::make_unique<Vector2D>(0, 0), std::make_unique<Vector2D>(0, 0)));

          // Add default button states for this joystick
          boost::container::small_vector<bool, 16> tempButtons;
          for (int j = 0; j < SDL_GAMEPAD_BUTTON_COUNT; j++) {
            tempButtons.push_back(false);
          }
          m_buttonStates.push_back(tempButtons);
        } else {
          std::cerr << "Forge Game Engine - Could not open gamepad: " << SDL_GetError() << std::endl;
          return;
        }
      }
    }
  } else {
    std::cout << "Forge Game Engine - No gamepads connected.\n";
    return; //return without setting m_gamePadInitialized to true.
  }

  m_gamePadInitialized = true;
}

void InputManager::reset() {
  m_mouseButtonStates[LEFT] = false;
  m_mouseButtonStates[RIGHT] = false;
  m_mouseButtonStates[MIDDLE] = false;
}

bool InputManager::isKeyDown(SDL_Scancode key) const {
  if (m_keystates != nullptr) {
    return m_keystates[key] == 1;
  }
  return false;
}

int InputManager::getAxisX(int joy, int stick) const {
  if (joy < 0 || joy >= static_cast<int>(m_joystickValues.size())) {
    return 0;
  }

  if (stick == 1) {
    return m_joystickValues[joy].first->getX();
  } else if (stick == 2) {
    return m_joystickValues[joy].second->getX();
  }

  return 0;
}

int InputManager::getAxisY(int joy, int stick) const {
  if (joy < 0 || joy >= static_cast<int>(m_joystickValues.size())) {
    return 0;
  }

  if (stick == 1) {
    return m_joystickValues[joy].first->getY();
  } else if (stick == 2) {
    return m_joystickValues[joy].second->getY();
  }

  return 0;
}

bool InputManager::getButtonState(int joy, int buttonNumber) const {
  if (joy < 0 || joy >= static_cast<int>(m_buttonStates.size()) ||
      buttonNumber < 0 ||
      buttonNumber >= static_cast<int>(m_buttonStates[joy].size())) {
    return false;
  }

  return m_buttonStates[joy][buttonNumber];
}

bool InputManager::getMouseButtonState(int buttonNumber) const {
  if (buttonNumber < 0 ||
      buttonNumber >= static_cast<int>(m_mouseButtonStates.size())) {
    return false;
  }

  return m_mouseButtonStates[buttonNumber];
}

Vector2D* InputManager::getMousePosition() const {
  return m_mousePosition.get();
}

void InputManager::update() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        std::cout << "Forge Game Engine - Shutting down! {}===]>" << std::endl;
        GameEngine::Instance().setRunning(false);
        break;

      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        onGamepadAxisMove(event);
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        onGamepadButtonDown(event);
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        onGamepadButtonUp(event);
        break;

      case SDL_EVENT_MOUSE_MOTION:
        onMouseMove(event);
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        onMouseButtonDown(event);
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        onMouseButtonUp(event);
        break;

      case SDL_EVENT_KEY_DOWN:
        onKeyDown(event);
        break;

      case SDL_EVENT_KEY_UP:
        onKeyUp(event);
        break;

      default:
        break;
    }
  }
}

void InputManager::onKeyDown(SDL_Event& /*event*/) {
  // Store the keyboard state
  m_keystates = SDL_GetKeyboardState(0);

}

void InputManager::onKeyUp(SDL_Event& /*event*/) {
  // TODO may not bew needed and need to clean upStore the keyboard state
  m_keystates = SDL_GetKeyboardState(0);

  // Key-specific processing can be handled by game states
  // using the isKeyDown() method
}

void InputManager::onMouseMove(SDL_Event& event) {
  m_mousePosition->setX(event.motion.x);
  m_mousePosition->setY(event.motion.y);
}

void InputManager::onMouseButtonDown(SDL_Event& event) {
  if (event.button.button == SDL_BUTTON_LEFT) {
    m_mouseButtonStates[LEFT] = true;
    std::cout << "Forge Game Engine - Mouse button Left clicked!\n";
  }
  if (event.button.button == SDL_BUTTON_MIDDLE) {
    m_mouseButtonStates[MIDDLE] = true;
    std::cout << "Forge Game Engine - Mouse button Middle clicked!\n";
  }
  if (event.button.button == SDL_BUTTON_RIGHT) {
    m_mouseButtonStates[RIGHT] = true;
    std::cout << "Forge Game Engine - Mouse button Right clicked!\n";
  }
}

void InputManager::onMouseButtonUp(SDL_Event& event) {
  if (event.button.button == SDL_BUTTON_LEFT) {
    m_mouseButtonStates[LEFT] = false;
  }
  if (event.button.button == SDL_BUTTON_MIDDLE) {
    m_mouseButtonStates[MIDDLE] = false;
  }
  if (event.button.button == SDL_BUTTON_RIGHT) {
    m_mouseButtonStates[RIGHT] = false;
  }
}

void InputManager::onGamepadAxisMove(SDL_Event& event) {
  int whichOne = 0;

  // Find which gamepad this event belongs to
  for (size_t i = 0; i < m_joysticks.size(); i++) {
    // In SDL3, use SDL_GetGamepadID directly
    if (SDL_GetGamepadID(m_joysticks[i]) == event.gaxis.which) {
      whichOne = static_cast<int>(i);
      break;
    }
  }

  // Make sure the gamepad index is valid
  if (whichOne >= static_cast<int>(m_joystickValues.size())) {
    return;
  }

  // Get axis name
  std::string axisName = "Unknown";
  switch (event.gaxis.axis) {
    case 0: axisName = "Left Stick X"; break;
    case 1: axisName = "Left Stick Y"; break;
    case 2: axisName = "Right Stick X"; break;
    case 3: axisName = "Right Stick Y"; break;
    case 4: axisName = "Left Trigger"; break;
    case 5: axisName = "Right Trigger"; break;
    default: axisName = "Unknown Axis";
  }

  // Process left stick X-axis
  if (event.gaxis.axis == 0) {
    if (event.gaxis.value > m_joystickDeadZone) {
      m_joystickValues[whichOne].first->setX(1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving RIGHT!\n";
    } else if (event.gaxis.value < -m_joystickDeadZone) {
      m_joystickValues[whichOne].first->setX(-1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving LEFT!\n";
    } else {
      m_joystickValues[whichOne].first->setX(0);
    }
  }

  // Process left stick Y-axis
  if (event.gaxis.axis == 1) {
    if (event.gaxis.value > m_joystickDeadZone) {
      m_joystickValues[whichOne].first->setY(1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving DOWN!\n";
    } else if (event.gaxis.value < -m_joystickDeadZone) {
      m_joystickValues[whichOne].first->setY(-1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving UP!\n";
    } else {
      m_joystickValues[whichOne].first->setY(0);
    }
  }

  // Process right stick X-axis
  if (event.gaxis.axis == 2) {
    if (event.gaxis.value > m_joystickDeadZone) {
      m_joystickValues[whichOne].second->setX(1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving RIGHT!\n";
    } else if (event.gaxis.value < -m_joystickDeadZone) {
      m_joystickValues[whichOne].second->setX(-1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving LEFT!\n";
    } else {
      m_joystickValues[whichOne].second->setX(0);
    }
  }

  // Process right stick Y-axis
  if (event.gaxis.axis == 3) {
    if (event.gaxis.value > m_joystickDeadZone) {
      m_joystickValues[whichOne].second->setY(1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving DOWN!\n";
    } else if (event.gaxis.value < -m_joystickDeadZone) {
      m_joystickValues[whichOne].second->setY(-1);
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " moving UP!\n";
    } else {
      m_joystickValues[whichOne].second->setY(0);
    }
  }

  // Process left trigger (L2/LT)
  if (event.gaxis.axis == 4) {
    if (event.gaxis.value > m_joystickDeadZone) {
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " pressed: " << event.gaxis.value << "\n";
    }
  }

  // Process right trigger (R2/RT)
  if (event.gaxis.axis == 5) {
    if (event.gaxis.value > m_joystickDeadZone) {
      std::cout << "Forge Game Engine - Gamepad " << whichOne << " - " << axisName << " pressed: " << event.gaxis.value << "\n";
    }
  }
}

void InputManager::onGamepadButtonDown(SDL_Event& event) {
  int whichOne = 0;

  // Find which gamepad this event belongs to
  for (size_t i = 0; i < m_joysticks.size(); i++) {
    // In SDL3, use SDL_GetGamepadID directly
    if (SDL_GetGamepadID(m_joysticks[i]) == event.gdevice.which) {
      whichOne = static_cast<int>(i);
      break;
    }
  }

  // Make sure the gamepad and button indices are valid
  if (whichOne >= static_cast<int>(m_buttonStates.size()) ||
      event.gbutton.button >=
          static_cast<int>(m_buttonStates[whichOne].size())) {
    return;
  }

  m_buttonStates[whichOne][event.gbutton.button] = true;

  // Get button name based on the button index
  std::string buttonName = "Unknown";
  switch (event.gbutton.button) {
    case 0: buttonName = "A or CROSS"; break;
    case 1: buttonName = "B or CIRCLE"; break;
    case 2: buttonName = "X or SQUARE"; break;
    case 3: buttonName = "Y or TRIANGLE"; break;
    case 4: buttonName = "Back"; break;
    case 5: buttonName = "Guide"; break;
    case 6: buttonName = "Start"; break;
    case 7: buttonName = "Left Stick"; break;
    case 8: buttonName = "Right Stick"; break;
    case 9: buttonName = "Left Shoulder"; break;
    case 10: buttonName = "Right Shoulder"; break;
    case 11: buttonName = "D-Pad Up"; break;
    case 12: buttonName = "D-Pad Down"; break;
    case 13: buttonName = "D-Pad Left"; break;
    case 14: buttonName = "D-Pad Right"; break;
    default: buttonName = "Unknown";
  }

  // Debug message for button press with button name
  std::cout << "Forge Game Engine - Gamepad " << whichOne << " Button '" << buttonName << "' ("
            << static_cast<int>(event.gbutton.button) << ") pressed!\n";
}

void InputManager::onGamepadButtonUp(SDL_Event& event) {
  int whichOne = 0;

  // Find which gamepad this event belongs to
  for (size_t i = 0; i < m_joysticks.size(); i++) {
    // In SDL3, use SDL_GetGamepadID directly
    if (SDL_GetGamepadID(m_joysticks[i]) == event.gdevice.which) {
      whichOne = static_cast<int>(i);
      break;
    }
  }

  // Make sure the gamepad and button indices are valid
  if (whichOne >= static_cast<int>(m_buttonStates.size()) ||
      event.gbutton.button >=
          static_cast<int>(m_buttonStates[whichOne].size())) {
    return;
  }

  m_buttonStates[whichOne][event.gbutton.button] = false;
}

void InputManager::clean() {
  if(m_gamePadInitialized) {
    int gamepadCount{0};
    // Close all gamepads if detected
    for (auto& gamepad : m_joysticks) {
      SDL_CloseGamepad(gamepad);
    gamepadCount++;
  }

  // No need to delete joystick values - smart pointers handle it
  // m_joystickValues will be cleared below

  m_joysticks.clear();
  m_joystickValues.clear();
  SDL_free(g_gamepadIDs);
  g_gamepadIDs = nullptr;
  m_gamePadInitialized = false;
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
  std::cout << "Forge Game Engine - " << gamepadCount << " gamepads freed!\n";
  std::cout << "Forge Game Engine - InputManager resources cleaned!\n";

}else{

    //m_buttonStates.clear(); cleared in destructor
    std::cout << "Forge Game Engine - no gamepads to free!\n";
    std::cout << "Forge Game Engine - InputManager resources cleaned!\n";
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

}
