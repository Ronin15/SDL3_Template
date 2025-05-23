# AI System Performance Optimizations

This document details the performance optimizations implemented in the AI system to ensure efficient operation with large numbers of entities.

## Current Optimizations

### 1. Distance-Based Update Frequency

Entities are updated at different frequencies based on their distance from the player, saving significant CPU resources.

#### How It Works

- **Close Range** (< `maxUpdateDistance`): Updated every frame
- **Medium Range** (< `mediumUpdateDistance`): Updated every 3 frames
- **Far Range** (< `minUpdateDistance`): Updated every 5 frames
- **Very Far** (beyond all thresholds): Updated every 10 frames

The actual update frequency is also influenced by the behavior's priority level, with higher priority behaviors (priority > 8) always being updated regardless of distance. The system uses a dynamic player-finding mechanism that works across different game states. A test-friendly fallback to distance-from-origin is automatically used when the player entity cannot be found.

```cpp
// Set custom update distances for a behavior
myBehavior->setMaxUpdateDistance(800.0f);      // Close range
myBehavior->setMediumUpdateDistance(1600.0f);  // Medium range
myBehavior->setMinUpdateDistance(2400.0f);     // Far range

// Or set all at once
myBehavior->setUpdateDistances(800.0f, 1600.0f, 2400.0f);

// Set priority (0-9, higher = more important)
myBehavior->setPriority(8);  // High priority behavior
```

#### Implementation Details

The optimization is implemented in the `AIBehavior::shouldUpdate()` method, which:

1. Checks if the behavior is active
2. Verifies update frequency requirements
3. For high-priority behaviors (priority > 8), always updates
4. Attempts to locate the player entity via the `findPlayerEntity()` method
5. If player is found, calculates distance between the entity and the player
6. If player is not found, falls back to using distance from origin
7. Determines update frequency based on the calculated distance and priority

#### Fallback Mechanism

The `findPlayerEntity()` method attempts to locate the player entity in the current game state. If no player entity is found (e.g., during tests or when a game state doesn't have a player), the system automatically falls back to using distance from origin. This provides a robust mechanism that works in all contexts.

#### Performance Impact

In typical game scenarios with 100+ AI entities, this optimization can reduce CPU usage by 40-60% compared to updating all entities every frame.

### 2. Memory Preallocation

The AI system preallocates memory for its internal containers to reduce runtime allocations and fragmentation.

#### Preallocated Containers

- Behavior registry (up to 20 behavior types)
- Entity-behavior assignments (up to 100 entities)
- Entity-behavior cache (up to 100 cached references)
- Behavior batches (up to 20 batches)
- Message queues (up to 128 messages)

#### Implementation Details

The preallocation happens in the `AIManager::init()` method:

```cpp
m_behaviors.reserve(20);          // Behavior types
m_entityBehaviors.reserve(100);   // Entity assignments
m_entityBehaviorCache.reserve(100); // Entity-behavior cache
m_behaviorBatches.reserve(20);    // Behavior batches
m_incomingMessageQueue.reserve(128); // Message queue
```

#### Performance Impact

Memory preallocation reduces memory fragmentation and eliminates performance hitches caused by container reallocations during gameplay. The impact is most noticeable during rapid entity creation/destruction scenarios.

## Additional Optimizations (Already Implemented)

### 3. Batch Processing

Entities with the same behavior type are processed in batches, improving cache locality.

### 4. Multi-threading Support

The AI system automatically detects available CPU cores and distributes batch updates across worker threads.

### 5. Double-Buffered Message Queue

The messaging system uses a double-buffered approach to reduce lock contention when sending and processing messages.

## Planned Future Optimizations

### 1. Spatial Partitioning

Implement a spatial grid system to quickly find entities in a specific area, reducing the cost of proximity queries.

### 2. Behavior Composition

Create a component-based behavior system to allow more code reuse and better composition of complex behaviors.

### 3. Lock-Free Messaging

Replace the current mutex-based message queue with a lock-free implementation to reduce thread synchronization overhead.

## Benchmarking

To measure the impact of AI optimizations, run the AI benchmark tool:

```
./run_ai_tests.sh
```

This will output performance metrics for various entity counts and AI complexity levels.

## Fine-Tuning Guidelines

When fine-tuning AI performance:

1. Adjust update distances based on your game's scale and player movement speed
2. Set priorities above 8 for critical behaviors that need to update every frame
3. Use lower update frequencies for non-critical background behaviors
4. Consider extending the GameState base class with a `getPlayer()` method for more consistent player access across different states
5. Test your AI behaviors with and without a player entity to ensure the fallback mechanism works correctly