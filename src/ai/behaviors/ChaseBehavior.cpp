/* Copyright (c) 2025 Hammer Forged Games
 * All rights reserved.
 * Licensed under the MIT License - see LICENSE file for details
*/

#include "ai/behaviors/ChaseBehavior.hpp"

ChaseBehavior::ChaseBehavior(EntityPtr target, float chaseSpeed, float maxRange, float minRange)
    : m_targetWeak(target), 
      m_chaseSpeed(chaseSpeed), 
      m_maxRange(maxRange), 
      m_minRange(minRange),
      m_isChasing(false), 
      m_hasLineOfSight(false), 
      m_lastKnownTargetPos(0, 0), 
      m_timeWithoutSight(0),
      m_maxTimeWithoutSight(60),
      m_currentDirection(0, 0) {
}

void ChaseBehavior::init(EntityPtr entity) {
    if (!entity) return;

    // Initialize the entity's state for chasing
    m_isChasing = false;
    m_hasLineOfSight = false;

    // If we have a target, check if it's in range
    auto target = m_targetWeak.lock();
    if (target) {
        Vector2D entityPos = entity->getPosition();
        Vector2D targetPos = target->getPosition();
        float distance = (targetPos - entityPos).length();

        m_isChasing = (distance <= m_maxRange);
        m_hasLineOfSight = checkLineOfSight(entity, target);
    }
}

void ChaseBehavior::update(EntityPtr entity) {
    if (!entity || !m_active) {
        return;
    }
    
    // Handle null target safely
    auto target = m_targetWeak.lock();
    if (!target) {
        // No target, so stop chasing
        if (entity) {
            entity->setVelocity(Vector2D(0, 0));
        }
        m_isChasing = false;
        return;
    }

    // Get positions
    Vector2D entityPos = entity->getPosition();
    Vector2D targetPos = target->getPosition();

    // Calculate distance to target and ensure we have a valid target position
    Vector2D toTarget = targetPos - entityPos;
    float distance = toTarget.length();

    // Store current target position for debugging
    m_lastKnownTargetPos = targetPos;

    // Check if target is in chase range
    if (distance <= m_maxRange) {
        // Check line of sight (simplified)
        m_hasLineOfSight = checkLineOfSight(entity, target);

        if (m_hasLineOfSight) {
            m_isChasing = true;

            // If not too close, move toward target
            if (distance > m_minRange) {
                // Normalize the direction
                Vector2D direction = toTarget * (1.0f / distance);

                // Set velocity toward target with consistent speed
                Vector2D newVelocity = direction * m_chaseSpeed;
                entity->setVelocity(newVelocity);

                // NPC class now handles sprite flipping based on velocity
            } else {
                // Target is within minimum range, stop moving
                entity->setVelocity(Vector2D(0, 0));

                onTargetReached(entity);
            }
        } else {
            // Lost line of sight
            if (m_isChasing) {
                // Continue to last known position for a while
                m_lastKnownTargetPos = targetPos;
                m_timeWithoutSight = 0;
            }

            handleNoLineOfSight(entity);
        }
    } else {
        // Target out of range, stop chasing
        if (m_isChasing) {
            m_isChasing = false;
            entity->setVelocity(Vector2D(0, 0));

            onTargetLost(entity);
        }
    }
}

void ChaseBehavior::clean(EntityPtr entity) {
    // Stop the entity's movement when cleaning up
    if (entity) {
        entity->setVelocity(Vector2D(0, 0));
    }
    
    // Reset all state
    m_isChasing = false;
    m_hasLineOfSight = false;
    m_lastKnownTargetPos = Vector2D(0, 0);
    m_timeWithoutSight = 0;
    
    // Important: don't hold onto the target after clean
    m_targetWeak.reset();
}

void ChaseBehavior::onMessage(EntityPtr entity, const std::string& message) {
    if (message == "pause") {
        setActive(false);
        if (entity) {
            entity->setVelocity(Vector2D(0, 0));
        }
    } else if (message == "resume") {
        setActive(true);
        // Reinitialize chase state when resuming
        if (entity && !m_targetWeak.expired()) {
            init(entity);
        }
    } else if (message == "lose_target") {
        m_isChasing = false;
        m_hasLineOfSight = false;
        if (entity) {
            entity->setVelocity(Vector2D(0, 0));
        }
    } else if (message == "release_entities") {
        // Clear target and reset state when asked to release entities
        m_targetWeak.reset();
        m_isChasing = false;
        m_hasLineOfSight = false;
        m_lastKnownTargetPos = Vector2D(0, 0);
        m_timeWithoutSight = 0;
        if (entity) {
            entity->setVelocity(Vector2D(0, 0));
        }
    }
}

std::string ChaseBehavior::getName() const {
    return "Chase";
}

void ChaseBehavior::setTarget(EntityPtr target) {
    // Always reset chase state when target changes
    m_isChasing = false;
    m_hasLineOfSight = false;
    m_lastKnownTargetPos = Vector2D(0, 0);
    m_timeWithoutSight = 0;
    
    // Set new target
    m_targetWeak = target;
}

EntityPtr ChaseBehavior::getTarget() const {
    return m_targetWeak.lock();
}

void ChaseBehavior::setChaseSpeed(float speed) {
    m_chaseSpeed = speed;
}

void ChaseBehavior::setMaxRange(float range) {
    m_maxRange = range;
}

void ChaseBehavior::setMinRange(float range) {
    m_minRange = range;
}

bool ChaseBehavior::isChasing() const {
    return m_isChasing;
}

bool ChaseBehavior::hasLineOfSight() const {
    return m_hasLineOfSight;
}

void ChaseBehavior::onTargetReached(EntityPtr entity) {
    // Base implementation does nothing
    // Override in derived behaviors for specific actions
    (void)entity; // Mark parameter as intentionally unused
}

void ChaseBehavior::onTargetLost(EntityPtr entity) {
    // Base implementation does nothing
    // Override in derived behaviors for specific actions
    (void)entity; // Mark parameter as intentionally unused
}

bool ChaseBehavior::checkLineOfSight(EntityPtr entity, EntityPtr target) {
    // For a more complex implementation, you would do raycasting here
    // This simplified version just checks distance
    if (!entity || !target) return false;

    Vector2D entityPos = entity->getPosition();
    Vector2D targetPos = target->getPosition();
    float distance = (targetPos - entityPos).length();

    // Always report in range for testing purposes
    // Simple line of sight check - would be replaced with raycasting in a real game
    return distance <= m_maxRange;
}

void ChaseBehavior::handleNoLineOfSight(EntityPtr entity) {
    if (m_timeWithoutSight < m_maxTimeWithoutSight) {
        // Move toward last known position for a while
        Vector2D entityPos = entity->getPosition();
        Vector2D toLastKnown = m_lastKnownTargetPos - entityPos;
        float distance = toLastKnown.length();

        if (distance > 10.0f) {
            // Still moving to last known position
            Vector2D direction = toLastKnown * (1.0f / distance);

            // Use a slightly slower speed when tracking last known position
            Vector2D newVelocity = direction * (m_chaseSpeed * 0.8f);
            entity->setVelocity(newVelocity);

            // NPC class now handles sprite flipping based on velocity
        } else {
            // Reached last known position, stop
            entity->setVelocity(Vector2D(0, 0));
        }

        m_timeWithoutSight++;
    } else {
        // Give up chase after timeout
        m_isChasing = false;
        entity->setVelocity(Vector2D(0, 0));

        onTargetLost(entity);
    }
}
