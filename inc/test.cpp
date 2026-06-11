/**
 * @file test_interactive.cpp
 * @brief Interactive sandbox for testing EventBus dispatching and memory safety
 */

#include <iostream>
#include <string>
#include "EventBus.hpp"
#include "Events.hpp"

/**
 * @brief Clears the input buffer to prevent infinite loops on bad input
 * @param None
 * @retval None
 */
void ClearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(10000, '\n');
}

int main() {
    EventBus bus;

    std::cout << "---- INITIALIZING SYSTEMS ----\n";

    // Physics System: Listens to Trigger Interceptions
    HandlerId physInterceptId = bus.Subscribe<InterceptionEvent>([](const InterceptionEvent& e) {
        std::string actStr = (e.action == TriggerAction::ENTER) ? "ENTERED" : 
                             (e.action == TriggerAction::EXIT) ? "EXITED" : "STAYING IN";
        std::cout << "  [PhysicsSystem] Collision detected: Entity " << e.entityA 
                  << " " << actStr << " trigger zone " << e.entityB << "\n";
    });

    // Quest System: Listens to Trigger Interceptions to start quests
    HandlerId questInterceptId = bus.Subscribe<InterceptionEvent>([&bus](const InterceptionEvent& e) {
        std::cout << "  [QuestSystem] Analyzing trigger overlap for Entity " << e.entityA << "\n";
        
        // Simulating logic: if player (ID 1) enters the quest item trigger (ID 999)
        if (e.entityA == 1 && e.entityB == 999 && e.action == TriggerAction::ENTER) {
            std::cout << "  [QuestSystem] Quest item reached! Emitting QuestStartEvent\n";
            // Publishing a new event INSIDE a handler to test safety
            bus.Publish(QuestStartEvent(42, 1)); 
        }
    });

    // Quest System: Listens to actual Quest Starts
    HandlerId questStartId = bus.Subscribe<QuestStartEvent>([](const QuestStartEvent& e) {
        std::cout << "  [QuestSystem] EXECUTE QUEST #" << e.questID 
                  << " for Entity " << e.entityID << "!\n";
    });

    // Player Control System: Listens to Input
    HandlerId inputId = bus.Subscribe<InputEvent>([](const InputEvent& e) {
        std::cout << "  [PlayerControlSystem] Processing key code: " << e.key 
                  << " | Action: " << e.action << "\n";
    });

    std::cout << "Systems registered successfully\n\n";

    bool isRunning = true;
    bool isPhysicsActive = true;

    while (isRunning) {
        std::cout << "\n-----------------------------------------\n";
        std::cout << " ENGINE DEBUG MENU (Awaiting Action)\n";
        std::cout << "-----------------------------------------\n";
        std::cout << " [1] Simulate: Player (ID: 1) enters Quest Item zone (ID: 999)\n";
        std::cout << " [2] Simulate: Player (ID: 1) exits Quest Item zone (ID: 999)\n";
        std::cout << " [3] Simulate: Key 'E' pressed (InputEvent)\n";
        std::cout << " [4] DISPATCH EVENTS (Simulate Frame End)\n";
        std::cout << "-----------------------------------------\n";
        if (isPhysicsActive) {
            std::cout << " [5] UNSUBSCRIBE PhysicsSystem from InterceptionEvent\n";
        } else {
            std::cout << " [5] (PhysicsSystem is already unsubscribed)\n";
        }
        std::cout << " [0] EXIT TEST\n";
        std::cout << "Select action: ";

        int choice;
        if (!(std::cin >> choice)) {
            ClearInputBuffer();
            continue;
        }

        std::cout << "\n";

        switch (choice) {
            case 1:
                std::cout << ">>> Pushing InterceptionEvent (ENTER)\n";
                bus.Publish(InterceptionEvent(1, 999, TriggerAction::ENTER));
                break;
            case 2:
                std::cout << ">>> Pushing InterceptionEvent (EXIT)\n";
                bus.Publish(InterceptionEvent(1, 999, TriggerAction::EXIT));
                break;
            case 3:
                std::cout << ">>> Pushing InputEvent (Key: 69 - 'E')\n";
                bus.Publish(InputEvent(69, INPUT_PRESS));
                break;
            case 4:
                std::cout << ">>> --- DISPATCHING FRAME EVENTS --- <<<\n";
                bus.DispatchEvents();
                std::cout << ">>> --- DISPATCH COMPLETE --- <<<\n";
                break;
            case 5:
                if (isPhysicsActive) {
                    std::cout << ">>> Unsubscribing PhysicsSystem (Handler ID: " << physInterceptId << ")\n";
                    bus.Unsubscribe(physInterceptId);
                    isPhysicsActive = false;
                    std::cout << ">>> Success. PhysicsSystem will no longer react to triggers\n";
                } else {
                    std::cout << ">>> Already unsubscribed!\n";
                }
                break;
            case 0:
                isRunning = false;
                std::cout << ">>> Exiting\n";
                break;
            default:
                std::cout << ">>> Unknown command\n";
                break;
        }
    }
}