# Cheese Engine

Cheese Engine is a custom 2D game engine written in modern C++. The project was created as a research-oriented engine focused on understanding the architecture of game engines, rendering pipelines and core game development concepts from scratch.

The engine provides a modular foundation for developing 2D games. It is designed with maintainability and extensibility in mind, using object-oriented principles and modern C++ practices. Cheese Engine serves as a framework for experimenting with engine development technologies.

---

## Navigation


- [Cheese Engine](#cheese-engine)
  - [Navigation](#navigation)
  - [Features](#features)
  - [How to Install and Run](#how-to-install-and-run)
    - [AI Setup \& Configuration](#ai-setup--configuration)
      - [1. Obtain your API Key](#1-obtain-your-api-key)
      - [2. Configure the Plugin](#2-configure-the-plugin)
      - [3. Initialize Continue](#3-initialize-continue)
  - [Technology Stack](#technology-stack)
  - [Authors](#authors)

---


## Features

* Modern C++ architecture
* Modular engine structure
* Window management
* Event and input handling
* Rendering system
* Cross-platform support

---

## How to Install and Run

 - install the latest release package
 - install the latest shaders archive
 - unpack shaders dir in your project dir
 - install [entt](https://github.com/skypjack/entt) and [glm](https://github.com/g-truc/glm) from oficial websites or github
 - add to your CMakeLists:
```cmake
target_include_directories({YOUR_TARGET}
    PRIVATE
        /usr/include # or replace usr with your installation path
)

target_link_directories(YOUR_TARGET}
    PRIVATE
        /usr/lib     # or replace usr with your installation path
)

target_link_libraries(YOUR_TARGET}
    PRIVATE
        CheeseEngine
)
```

---
### AI Setup & Configuration

To enable the AI-assisted development features (powered by Google Gemini), follow these steps to configure the **Continue** plugin for **VS Code**.

#### 1. Obtain your API Key

1. Visit the [Google AI Studio](https://aistudio.google.com/).
2. Sign in with your Google account.
3. Click on **"Get API key"** in the left bar.
4. Create an API key in a new or existing project and copy it.

#### 2. Configure the Plugin

1. Open your project in **Visual Studio Code**.
2. Locate the `.continue/agents/config.yaml` file in your project directory.
3. Open the file and find the `apiKey` field within the provider configuration for Google Gemini:
```yaml
models:
  - name: "Gemini 3.1 Flash"
    provider: gemini
    model: gemini-3.1-flash-lite
    apiKey: "<YOUR_API_KEY>" # Your API key from Google AI Studio here

```

*Replace `YOUR_API_KEY` with the key you obtained in the previous step.*

#### 3. Initialize Continue

1. Install the **Continue** extension from the VS Code Marketplace.
2. Once installed, click the Continue icon in the sidebar.
3. If not detected automatically, click at the confings button in the top of Continue chat.
4. Choose config.yaml.
5. You are now ready to use the AI assistant with documentation context.

---


## Technology Stack

- **C++20** — core programming language
- **CMake** — cross-platform build automation system
- **Vulkan** — modern low-level graphics and rendering API
- **GLFW** — cross-platform window and input management library
- **Doxygen** — documentation generation from source code comments
- **Continue (VS Code Plugin)** — AI-powered development assistant and model integration platform
- **Google AI Studio** — Gemini API provisioning and model access management
- **Git** — distributed version control system
---

## Authors

- [Vladimir Zavorokhin](https://github.com/71darkness17)
- [Dmitriy Lepa](https://github.com/Fatummm)
- [Artem Shekhovtsov](https://github.com/Flichendery)