'''
# Falling-Sand Sandbox Demo

This project is a simple falling-sand sandbox simulation built with C++ and SDL2. It uses xmake for building the project.

## Prerequisites

- **xmake**: Follow the installation instructions at [https://xmake.io/#/guide/installation](https://xmake.io/#/guide/installation)
- **SDL2**: xmake will automatically download and link the SDL2 dependency, so no manual installation is required.

## Build and Run

1.  **Build the project:**

    ```bash
    xmake
    ```

2.  **Run the simulation:**

    ```bash
    xmake run sand
    ```

## Controls

-   **Left Mouse Drag**: Add sand to the simulation.
-   **Spacebar**: Pause or resume the simulation.
-   **C**: Clear the grid.
-   **Up Arrow**: Increase the brush size.
-   **Down Arrow**: Decrease the brush size.
-   **Escape**: Exit the application.

## Project Structure

-   `xmake.lua`: The build script for the project.
-   `src/main.cpp`: The main source file containing the simulation logic.
-   `README.md`: This file.
'''
