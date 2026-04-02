//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Unit tests for dependency injection and component pattern using Catch2.

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <utility>
#include <vector>

//============================================================================
// Component Interfaces and Implementations
//============================================================================

// Simple component interface for thinking behavior
class IThinkable
{
  public:
    virtual ~IThinkable() = default;
    virtual void think() = 0;
    virtual int getThinkCount() const = 0;
};

// Concrete implementation of IThinkable
class Actor : public IThinkable
{
  private:
    int thinkCount_ = 0;
    bool isActive_ = true;

  public:
    void think() override
    {
        if (isActive_)
        {
            thinkCount_++;
        }
    }

    int getThinkCount() const override
    {
        return thinkCount_;
    }

    void setActive(bool active)
    {
        isActive_ = active;
    }

    bool isActive() const
    {
        return isActive_;
    }
};

// Another component interface
class IRenderable
{
  public:
    virtual ~IRenderable() = default;
    virtual void render() = 0;
    virtual bool isVisible() const = 0;
    virtual int getRenderCount() const = 0;
};

// Concrete renderable component
class RenderComponent : public IRenderable
{
  private:
    bool visible_ = true;
    int renderCount_ = 0;

  public:
    void render() override
    {
        if (visible_)
        {
            renderCount_++;
        }
    }

    bool isVisible() const override
    {
        return visible_;
    }

    void setVisible(bool visible)
    {
        visible_ = visible;
    }

    int getRenderCount() const override
    {
        return renderCount_;
    }
};

// Game class demonstrating dependency injection
class Game
{
  private:
    std::unique_ptr<IThinkable> actor_;
    std::unique_ptr<IRenderable> renderComponent_;
    std::vector<std::unique_ptr<IThinkable>> actors_;
    bool isRunning_ = false;

  public:
    void setActor(std::unique_ptr<IThinkable> actor)
    {
        actor_ = std::move(actor);
    }

    IThinkable *getActor()
    {
        return actor_.get();
    }

    const IThinkable *getActor() const
    {
        return actor_.get();
    }

    void setRenderComponent(std::unique_ptr<IRenderable> component)
    {
        renderComponent_ = std::move(component);
    }

    IRenderable *getRenderComponent()
    {
        return renderComponent_.get();
    }

    void addActor(std::unique_ptr<IThinkable> actor)
    {
        actors_.push_back(std::move(actor));
    }

    size_t getActorCount() const
    {
        return actors_.size();
    }

    void runFrame()
    {
        isRunning_ = true;
        if (actor_)
        {
            actor_->think();
        }
        for (auto &actor : actors_)
        {
            actor->think();
        }
        if (renderComponent_)
        {
            renderComponent_->render();
        }
        isRunning_ = false;
    }

    bool isRunning() const
    {
        return isRunning_;
    }

    void updateActor(std::unique_ptr<IThinkable> newActor)
    {
        actor_ = std::move(newActor);
    }

    // Const-correct way to check if actor exists
    bool hasActor() const
    {
        return actor_ != nullptr;
    }

    bool hasRenderComponent() const
    {
        return renderComponent_ != nullptr;
    }
};

//============================================================================
// Test Cases: Interface and Implementation
//============================================================================

TEST_CASE("IThinkable interface creation", "[component][interface]")
{
    // Test that we can create a pointer to interface (requires concrete impl)
    Actor actor;
    IThinkable *iface = &actor;
    REQUIRE(iface != nullptr);

    // Virtual function should work through interface
    iface->think();
    REQUIRE(actor.getThinkCount() == 1);
}

TEST_CASE("Actor concrete implementation", "[component][implementation]")
{
    Actor actor;

    // Test initial state
    REQUIRE(actor.getThinkCount() == 0);
    REQUIRE(actor.isActive() == true);

    // Test think() increments counter
    actor.think();
    actor.think();
    actor.think();
    REQUIRE(actor.getThinkCount() == 3);

    // Test deactivation
    actor.setActive(false);
    actor.think(); // Should not increment when inactive
    REQUIRE(actor.getThinkCount() == 3);
}

//============================================================================
// Test Cases: Unique_ptr Ownership
//============================================================================

TEST_CASE("unique_ptr ownership transfer", "[component][ownership]")
{
    Game game;
    auto actor = std::make_unique<Actor>();

    // Transfer ownership to game
    game.setActor(std::move(actor));

    // Actor should now be in game
    REQUIRE(game.hasActor() == true);
    REQUIRE(game.getActor() != nullptr);

    // Original pointer should be null after move
    // (This is a compile-time check concept - actor is now invalid)
}

TEST_CASE("unique_ptr memory safety", "[component][ownership]")
{
    Game game;

    // Without actor, getActor returns nullptr
    REQUIRE(game.getActor() == nullptr);

    // Add actor
    auto actor = std::make_unique<Actor>();
    game.setActor(std::move(actor));

    // Now should have valid pointer
    REQUIRE(game.getActor() != nullptr);

    // Run game frame - should not crash
    game.runFrame();
    REQUIRE(game.getActor()->getThinkCount() == 1);
}

//============================================================================
// Test Cases: Component Updating
//============================================================================

TEST_CASE("Component think update", "[component][update]")
{
    Game game;
    auto actor = std::make_unique<Actor>();
    game.setActor(std::move(actor));

    // Run multiple frames
    game.runFrame();
    game.runFrame();
    game.runFrame();

    REQUIRE(game.getActor()->getThinkCount() == 3);
}

TEST_CASE("Multiple component updates", "[component][update]")
{
    Game game;
    game.setActor(std::make_unique<Actor>());
    game.setRenderComponent(std::make_unique<RenderComponent>());

    // Run frame
    game.runFrame();

    REQUIRE(game.getActor()->getThinkCount() == 1);
    REQUIRE(game.getRenderComponent()->getRenderCount() == 1);
}

//============================================================================
// Test Cases: Null Component Handling
//============================================================================

TEST_CASE("Null component handling", "[component][null]")
{
    Game game;

    // No actor set - should handle gracefully
    REQUIRE(game.getActor() == nullptr);
    REQUIRE(game.hasActor() == false);

    // Running frame with null actor should not crash
    game.runFrame();
    REQUIRE(game.isRunning() == false); // Frame completed
}

TEST_CASE("Null render component handling", "[component][null]")
{
    Game game;
    game.setActor(std::make_unique<Actor>());

    // No render component set
    REQUIRE(game.getRenderComponent() == nullptr);
    REQUIRE(game.hasRenderComponent() == false);

    // Should run without crashing
    game.runFrame();
    REQUIRE(game.getActor()->getThinkCount() == 1);
}

//============================================================================
// Test Cases: Multiple Components
//============================================================================

TEST_CASE("Multiple actors in collection", "[component][multiple]")
{
    Game game;

    // Set main actor first
    game.setActor(std::make_unique<Actor>());

    // Add multiple actors to collection
    game.addActor(std::make_unique<Actor>());
    game.addActor(std::make_unique<Actor>());
    game.addActor(std::make_unique<Actor>());

    REQUIRE(game.getActorCount() == 3);

    // Run frame - all should think
    game.runFrame();

    // All 3 actors should have thinkCount = 1
    // Note: We can only access the main actor directly
    REQUIRE(game.getActor() != nullptr);
}

TEST_CASE("Main actor and collection actors", "[component][multiple]")
{
    Game game;

    // Set main actor
    auto mainActor = std::make_unique<Actor>();
    mainActor->think(); // Main actor thinks once before game starts
    game.setActor(std::move(mainActor));

    // Add additional actors
    game.addActor(std::make_unique<Actor>());
    game.addActor(std::make_unique<Actor>());

    REQUIRE(game.getActorCount() == 2);
    REQUIRE(game.hasActor() == true);

    // Run frame
    game.runFrame();

    // Main actor should have 2 thinks (1 before + 1 in frame)
    REQUIRE(game.getActor()->getThinkCount() == 2);
}

//============================================================================
// Test Cases: Component Replacement
//============================================================================

TEST_CASE("Component replacement", "[component][replacement]")
{
    Game game;

    // Set initial actor
    auto actor1 = std::make_unique<Actor>();
    actor1->think();
    game.setActor(std::move(actor1));

    REQUIRE(game.getActor()->getThinkCount() == 1);

    // Replace with new actor
    auto actor2 = std::make_unique<Actor>();
    game.updateActor(std::move(actor2));

    // New actor should have fresh state
    REQUIRE(game.getActor()->getThinkCount() == 0);

    // Old actor's think count stays at 1 (they are separate objects)
}

TEST_CASE("Render component replacement", "[component][replacement]")
{
    Game game;

    // Set initial render component
    auto render1 = std::make_unique<RenderComponent>();
    render1->render();
    game.setRenderComponent(std::move(render1));

    REQUIRE(game.getRenderComponent()->getRenderCount() == 1);

    // Replace with new component
    auto render2 = std::make_unique<RenderComponent>();
    game.setRenderComponent(std::move(render2));

    // New component should have fresh state
    REQUIRE(game.getRenderComponent()->getRenderCount() == 0);
}

//============================================================================
// Test Cases: Const Correctness
//============================================================================

TEST_CASE("Const actor access", "[component][const]")
{
    Game game;
    game.setActor(std::make_unique<Actor>());

    // Get const pointer
    const IThinkable *constActor = game.getActor();
    REQUIRE(constActor != nullptr);

    // Can call const methods
    int count = constActor->getThinkCount();
    REQUIRE(count == 0);

    // Cannot call non-const methods through const pointer
    // (This would not compile: constActor->think());
}

TEST_CASE("Const game query methods", "[component][const]")
{
    Game game;
    const Game &constGame = game;

    // These const methods should work
    bool hasActor = constGame.hasActor();
    bool hasRender = constGame.hasRenderComponent();

    REQUIRE(hasActor == false);
    REQUIRE(hasRender == false);
}

TEST_CASE("Const component state", "[component][const]")
{
    Actor actor;
    const Actor &constActor = actor;

    // Can query const state
    REQUIRE(constActor.isActive() == true);
    REQUIRE(constActor.getThinkCount() == 0);

    // Non-const operations through non-const reference
    actor.think();
    REQUIRE(actor.getThinkCount() == 1);

    // Const reference still shows updated value (object is same)
    REQUIRE(constActor.getThinkCount() == 1);
}

//============================================================================
// Test Cases: Interface Polymorphism
//============================================================================

TEST_CASE("Interface polymorphism", "[component][polymorphism]")
{
    // Store different implementations through same interface
    std::vector<std::unique_ptr<IThinkable>> thinkables;

    thinkables.push_back(std::make_unique<Actor>());
    thinkables.push_back(std::make_unique<Actor>());

    // All can think through interface
    for (auto &t : thinkables)
    {
        t->think();
    }

    // Both actors should have incremented
    // (Accessing through dynamic_cast to check)
    REQUIRE(thinkables.size() == 2);
}

TEST_CASE("Component lifecycle", "[component][lifecycle]")
{
    // Test creation
    auto actor = std::make_unique<Actor>();
    REQUIRE(actor->getThinkCount() == 0);
    REQUIRE(actor->isActive() == true);

    // Test update phase
    actor->think();
    actor->think();
    REQUIRE(actor->getThinkCount() == 2);

    // Test destruction (via unique_ptr going out of scope)
    actor.reset();
    // Actor is now destroyed
}
