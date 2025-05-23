# AI Manager System Documentation

## Overview

The AI Manager is a centralized system for creating and managing autonomous behaviors for game entities. It provides a flexible framework for implementing and controlling various AI behaviors, allowing game entities to exhibit different movement patterns and reactions. The system includes performance optimizations such as:

1. Entity-behavior caching for faster lookups
2. Batch processing for entities with the same behavior
3. Early exit conditions to skip unnecessary updates
4. Message queue system for efficient communication

## Core Components

### AIManager

The central management class that handles:
- Registration of behaviors
- Assignment of behaviors to entities
- Updating all behaviors during the game loop
- Communication with behaviors via messages

**Performance Optimizations:**
- Entity-behavior caching for faster lookups
- Batch processing of entities with the same behavior
- Early exit conditions to avoid unnecessary updates
- Message queue system for deferred communication

### AIBehavior Base Class

The abstract interface that all behaviors must implement:
- `update(Entity*)`: Called each frame to update entity movement/actions
- `init(Entity*)`: Called when a behavior is first assigned to an entity
- `clean(Entity*)`: Called when a behavior is removed from an entity
- `onMessage(Entity*, const std::string&)`: Handles messages sent to the behavior

**Optimization Methods:**
- `shouldUpdate(Entity*)`: Early exit condition checking if entity should be updated
- `isEntityInRange(Entity*)`: Early exit condition checking if entity is in range
- `isWithinUpdateFrequency()`: Early exit condition for update frequency control
- `setUpdateFrequency(int)`: Sets how often the behavior should update (1=every frame)

## Available Behaviors

### WanderBehavior

Entities move randomly within a defined area, changing direction periodically.

**Configuration options:**
- Movement speed
- Direction change interval
- Wander area radius

### PatrolBehavior

Entities follow a predefined path of waypoints, moving from point to point in sequence.

**Configuration options:**
- Array of waypoint positions
- Movement speed
- Waypoint radius (how close to get before moving to next point)

### ChaseBehavior

Entities pursue a target (typically the player) when within detection range.

**Configuration options:**
- Target entity reference
- Chase speed
- Maximum detection/pursuit range
- Minimum distance to maintain from target

## Example Usage

### Basic Setup

```cpp
// AI Manager is initialization happens at the beginning of the starup sequence.
AIManager::Instance().init();

// Create and register behaviors
auto wanderBehavior = std::make_shared<WanderBehavior>(2.0f, 3000.0f, 200.0f);
AIManager::Instance().registerBehavior("Wander", wanderBehavior);

// Create patrol points
std::vector<Vector2D> patrolPoints;
patrolPoints.push_back(Vector2D(100, 100));
patrolPoints.push_back(Vector2D(500, 100));
patrolPoints.push_back(Vector2D(500, 400));
patrolPoints.push_back(Vector2D(100, 400));
auto patrolBehavior = std::make_shared<PatrolBehavior>(patrolPoints, 1.5f);
AIManager::Instance().registerBehavior("Patrol", patrolBehavior);

// Create chase behavior (targeting the player)
auto chaseBehavior = std::make_shared<ChaseBehavior>(player, 2.0f, 500.0f, 50.0f);
AIManager::Instance().registerBehavior("Chase", chaseBehavior);
```

### Assigning Behaviors to Entities

```cpp
// Create an NPC
auto npc = std::make_unique<NPC>("npc_sprite", Vector2D(250, 250), 64, 64);

// Assign an initial behavior
AIManager::Instance().assignBehaviorToEntity(npc.get(), "Wander");

// Later, switch to a different behavior
AIManager::Instance().assignBehaviorToEntity(npc.get(), "Patrol");

// Respond to player detection by switching to chase
if (isPlayerDetected) {
    AIManager::Instance().assignBehaviorToEntity(npc.get(), "Chase");
}
```

### Controlling Behaviors with Messages

```cpp
// Pause a specific entity's behavior (immediate delivery)
AIManager::Instance().sendMessageToEntity(npc.get(), "pause", true);

// Resume a specific entity's behavior (queued for next update)
AIManager::Instance().sendMessageToEntity(npc.get(), "resume");

// Pause all AI entities (immediate delivery)
AIManager::Instance().broadcastMessage("pause", true);

// Reverse the patrol route for a specific entity (queued for next update)
AIManager::Instance().sendMessageToEntity(npc.get(), "reverse");

// Manually process queued messages (normally happens automatically during update())
AIManager::Instance().processMessageQueue();
```

### Integration with Game Loop

To ensure behaviors are updated each frame, call the AIManager's update method in your game state's update method:

```cpp
void GamePlayState::update() {
    // Update all AI behaviors
    AIManager::Instance().update();

    // Your other game update code...
    player->update();
    checkCollisions();
    updateUI();
}
```

### Cleanup

When switching game states or shutting down, clean up the AI system:

```cpp
void GamePlayState::exit() {
    // Clean up AI Manager
    AIManager::Instance().clean();

    // Your other cleanup code...
}
```

## Creating Custom Behaviors

To create a custom behavior, inherit from the AIBehavior base class and implement the required methods:

```cpp
class FlankingBehavior : public AIBehavior {
public:
    FlankingBehavior(Entity* target, float speed = 2.0f, float flankDistance = 100.0f)
        : m_target(target), m_speed(speed), m_flankDistance(flankDistance) {}

    void init(Entity* entity) override {
        // Initialize behavior state
    }

    void update(Entity* entity) override {
        // Implement flanking movement logic
    }

    void clean(Entity* entity) override {
        // Clean up resources
    }

    std::string getName() const override {
        return "Flanking";
    }

private:
    Entity* m_target;
    float m_speed;
    float m_flankDistance;
};
```

## Threading Considerations

The AIManager optionally utilizes the ThreadSystem to distribute AI updates across multiple CPU cores. This is enabled by default but can be controlled through the AIManager's initialization:

```cpp
// Disable threading for AI updates (if needed for debugging)
AIManager::Instance().setUseThreading(false);
```

When threading is enabled, be careful about accessing shared resources from behavior update methods. Consider using locks or designing behaviors to be thread-safe. The thread system automatically manages task capacity, so you don't need to worry about managing task queue size.

## Performance Optimizations

### 1. Entity Component Caching

Entity-behavior relationships are cached for faster lookups during updates:

```cpp
// The cache is automatically maintained
// Force a cache rebuild if needed
AIManager::Instance().ensureOptimizationCachesValid();
```

### 2. Batch Processing

Entities with the same behavior are processed in batches for better cache coherency:

```cpp
// Create a vector of entities
std::vector<Entity*> enemyGroup = getEnemiesInSector();

// Process all entities with the same behavior in one batch
AIManager::Instance().batchProcessEntities("ChaseBehavior", enemyGroup);
```

### 3. Early Exit Conditions

Set early exit conditions to skip unnecessary updates:

```cpp
// Create a behavior that only updates every 3 frames
auto patrolBehavior = std::make_shared<PatrolBehavior>(patrolPoints, 1.5f);
patrolBehavior->setUpdateFrequency(3);
AIManager::Instance().registerBehavior("Patrol", patrolBehavior);

// In your custom behavior class:
bool YourBehavior::shouldUpdate([[maybe_unused]] Entity* entity) const override {
    float distanceToPlayer = entity->getPosition().distance(player->getPosition());
    return distanceToPlayer < 1000.0f; // Skip updates for distant entities
}
```

### 4. Message Queue System

Messages can be queued for batch processing instead of immediate delivery. The system uses an optimized double-buffered queue for better performance:

```cpp
// Queue a message (default) - processed during next update
try {
    AIManager::Instance().sendMessageToEntity(npc.get(), "patrol");
} catch (const std::exception& e) {
    std::cerr << "Failed to queue message: " << e.what() << std::endl;
}

// Send message immediately when needed
try {
    AIManager::Instance().sendMessageToEntity(npc.get(), "evade", true);
} catch (const std::exception& e) {
    std::cerr << "Failed to deliver message: " << e.what() << std::endl;
}

// Manually process all queued messages (normally done during update)
AIManager::Instance().processMessageQueue();
```

### Performance Tips

// Limit active behaviors: Only register and assign behaviors you're actively using.
2. **Optimize waypoints**: Use fewer waypoints for simple patrol routes.
3. **Adjust update frequency**: Use the built-in update frequency control for less important entities.
4. **Cull inactive entities**: Unassign behaviors from entities that are far from the player or inactive.
5. **Use batch processing**: Leverage the built-in batch processing for entities with the same behavior type.
6. **Use early exit conditions**: Configure behaviors to skip updates when not necessary.
7. **Queue non-urgent messages**: Use the message queue system for non-urgent communication.
8. **Add proper error handling**: Always wrap behavior code in try-catch blocks to prevent crashes.
9. **Use string_view parameters**: When possible, use std::string_view for string parameters to reduce copying.
10. **Examine performance statistics**: Use the built-in performance tracking to identify bottlenecks.

## Implementation Details

### Entity-Behavior Caching

The AIManager maintains an optimized cache of entity-behavior pairs:

```cpp
// Entity-behavior cache structure
struct EntityBehaviorCache {
    Entity* entity;
    AIBehavior* behavior;
    std::string_view behaviorName;  // Using string_view for better performance
    PerformanceStats perfStats;     // Performance tracking for each entity-behavior pair
};
```

This cache is automatically updated when:
- New behaviors are registered
- Behaviors are assigned to entities
- Behaviors are unassigned from entities

### Batch Processing Implementation

Batch processing improves performance by:
- Grouping entities by behavior type for better cache coherency
- Reducing per-entity overhead in update loops
- Enabling more efficient multithreading with similar workloads

```cpp
// Optimized batched update method
void AIManager::batchUpdateAllBehaviors();
```

### Early Exit System

The AIManager applies three levels of early exit checks:
1. **Update frequency** - Skip updates based on frame count
2. **Entity range check** - Skip updates for out-of-range entities
3. **Custom conditions** - Skip updates based on behavior-specific logic

### Message Queue System

The message queue system provides:
- Deferred message delivery for non-critical communications
- Batched processing of messages during update cycles
- Thread-safe message queue implementation with double-buffering
- Optimized memory handling with move semantics
- Performance statistics tracking for message processing
- Optional immediate delivery for time-critical messages

## API Reference

### Core AIManager Methods

```cpp
// Basic AIManager methods
bool init();
void update();
void clean();
void resetBehaviors();

// Behavior management
void registerBehavior(const std::string& behaviorName, std::shared_ptr<AIBehavior> behavior);
bool hasBehavior(const std::string& behaviorName) const;
AIBehavior* getBehavior(const std::string& behaviorName) const;
size_t getBehaviorCount() const;

// Entity-behavior assignment
void assignBehaviorToEntity(Entity* entity, const std::string& behaviorName);
void unassignBehaviorFromEntity(Entity* entity);
bool entityHasBehavior(Entity* entity) const;
size_t getManagedEntityCount() const;

// Optimization methods
void batchProcessEntities(const std::string& behaviorName, const std::vector<Entity*>& entities);
void batchUpdateAllBehaviors();
void ensureOptimizationCachesValid();

// Messaging system
void sendMessageToEntity(Entity* entity, const std::string& message, bool immediate = false);
void broadcastMessage(const std::string& message, bool immediate = false);
void processMessageQueue();
```

### AIBehavior Methods

```cpp
// Core behavior methods
virtual void update(Entity* entity) = 0;
virtual void init(Entity* entity) = 0;
virtual void clean(Entity* entity) = 0;
virtual std::string getName() const = 0;

// Optional message handling (with unused parameter attribute)
virtual void onMessage([[maybe_unused]] Entity* entity, 
                       [[maybe_unused]] const std::string& message);

// State management
virtual bool isActive() const;
virtual void setActive(bool active);
virtual int getPriority() const;
virtual void setPriority(int priority);

// Early exit optimizations (with unused parameter attribute)
virtual bool shouldUpdate([[maybe_unused]] Entity* entity) const;
virtual bool isEntityInRange([[maybe_unused]] Entity* entity) const;
virtual bool isWithinUpdateFrequency() const;
virtual void setUpdateFrequency(int framesPerUpdate);
virtual int getUpdateFrequency() const;
```

For the complete API details, see:
- `include/managers/AIManager.hpp`
- `include/ai/AIBehavior.hpp`
- `include/ai/behaviors/WanderBehavior.hpp`
- `include/ai/behaviors/PatrolBehavior.hpp`
- `include/ai/behaviors/ChaseBehavior.hpp`
