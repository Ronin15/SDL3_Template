/* Copyright (c) 2025 Hammer Forged Games
 * All rights reserved.
 * Licensed under the MIT License - see LICENSE file for details
*/

#include "core/TimestepManager.hpp"
#include <algorithm>

TimestepManager::TimestepManager(float targetFPS, float fixedTimestep)
    : m_targetFPS(targetFPS)
    , m_fixedTimestep(fixedTimestep)
    , m_targetFrameTime(1.0f / targetFPS)
    , m_accumulator(0.0)
    , m_lastFrameTimeMs(0)
    , m_currentFPS(0.0f)
    , m_frameCount(0)
    , m_shouldRender(true)
    , m_firstFrame(true)
{
    Uint64 currentTime = SDL_GetTicks();
    m_frameStart = currentTime;
    m_lastFrameTime = currentTime;
    m_fpsLastUpdate = currentTime;
    
    // Initialize fixed timestep state
    m_usingSoftwareFrameLimiting = false;
    m_explicitlySet = false;
}

void TimestepManager::startFrame() {
    Uint64 currentTime = SDL_GetTicks();
    
    if (m_firstFrame) {
        m_firstFrame = false;
        m_lastFrameTime = currentTime;
        m_frameStart = currentTime;
        return;
    }
    
    // Calculate frame delta time in seconds
    double deltaTimeMs = static_cast<double>(currentTime - m_lastFrameTime);
    m_lastFrameTime = currentTime;
    m_frameStart = currentTime;
    
    // Update frame time in milliseconds
    m_lastFrameTimeMs = static_cast<uint32_t>(deltaTimeMs);
    
    // Convert to seconds and clamp to prevent spiral of death
    double deltaTime = deltaTimeMs / 1000.0;
    deltaTime = std::min(deltaTime, static_cast<double>(MAX_ACCUMULATOR));
    
    // Add to accumulator for fixed timestep updates
    m_accumulator += deltaTime;
    
    // Always render once per frame
    m_shouldRender = true;
    
    // Update FPS counter
    updateFPS();
}

bool TimestepManager::shouldUpdate() {
    // Always update once per frame for responsive input
    // Use actual frame time instead of fixed timestep accumulator
    if (m_accumulator > 0.0) {
        m_accumulator = 0.0; // Reset for next frame
        return true;
    }
    return false;
}

bool TimestepManager::shouldRender() const {
    return m_shouldRender;
}

float TimestepManager::getUpdateDeltaTime() const {
    // For VSync, use actual frame time (it's already perfectly smooth)
    if (!m_usingSoftwareFrameLimiting) {
        return static_cast<float>(m_lastFrameTimeMs) / 1000.0f;
    }
    
    // For software frame limiting, use fixed timestep to eliminate ALL timing variations
    // This ensures perfectly consistent movement regardless of SDL_Delay() precision
    return m_fixedTimestep;
}

void TimestepManager::endFrame() {
    // Mark render as completed for this frame
    m_shouldRender = false;
    
    // Limit frame rate
    limitFrameRate();
    
    m_frameCount++;
}

float TimestepManager::getCurrentFPS() const {
    return m_currentFPS;
}

float TimestepManager::getTargetFPS() const {
    return m_targetFPS;
}

uint32_t TimestepManager::getFrameTimeMs() const {
    return m_lastFrameTimeMs;
}

bool TimestepManager::isFrameTimeExcessive() const {
    // Consider frame time excessive if it's more than 2x target frame time
    return m_lastFrameTimeMs > static_cast<uint32_t>(m_targetFrameTime * 2000.0f);
}

void TimestepManager::setTargetFPS(float fps) {
    if (fps > 0.0f) {
        m_targetFPS = fps;
        m_targetFrameTime = 1.0f / fps;
    }
}

void TimestepManager::setFixedTimestep(float timestep) {
    if (timestep > 0.0f) {
        m_fixedTimestep = timestep;
    }
}

void TimestepManager::reset() {
    m_accumulator = 0.0;
    m_frameCount = 0;
    m_firstFrame = true;
    m_shouldRender = true;
    m_currentFPS = 0.0f;
    
    Uint64 currentTime = SDL_GetTicks();
    m_frameStart = currentTime;
    m_lastFrameTime = currentTime;
    m_fpsLastUpdate = currentTime;
    
    // Preserve explicit software frame limiting settings during reset
    // Only reset if not explicitly configured by GameEngine
    if (!m_explicitlySet) {
        m_usingSoftwareFrameLimiting = false;
    }
}

void TimestepManager::updateFPS() {
    Uint64 currentTime = SDL_GetTicks();
    double timeSinceLastUpdateMs = static_cast<double>(currentTime - m_fpsLastUpdate);
    double timeSinceLastUpdate = timeSinceLastUpdateMs / 1000.0; // Convert to seconds
    
    // Update FPS calculation every second
    if (timeSinceLastUpdate >= 1.0) {
        m_currentFPS = static_cast<float>(m_frameCount) / static_cast<float>(timeSinceLastUpdate);
        m_frameCount = 0;
        m_fpsLastUpdate = currentTime;
    }
}

void TimestepManager::limitFrameRate() const {
    Uint64 currentTime = SDL_GetTicks();
    double frameTimeMs = static_cast<double>(currentTime - m_frameStart);
    double frameTime = frameTimeMs / 1000.0; // Convert to seconds
    
    // If we finished early, delay to meet target frame rate
    if (frameTime < m_targetFrameTime) {
        double sleepTime = m_targetFrameTime - frameTime;
        uint32_t sleepMs = static_cast<uint32_t>(sleepTime * 1000.0);
        
        if (sleepMs > 0) {
            SDL_Delay(sleepMs);
        }
    }
}

void TimestepManager::setSoftwareFrameLimiting(bool useSoftwareLimiting) {
    m_usingSoftwareFrameLimiting = useSoftwareLimiting;
    m_explicitlySet = true;
}

