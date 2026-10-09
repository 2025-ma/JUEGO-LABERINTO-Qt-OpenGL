# Ghost in the Maze

**3D Maze Video Game Developed with Qt and OpenGL**

## Project Description

Ghost in the Maze is a 3D video game in which the player must explore a maze, collect 10 coins, and reach the exit without being caught by the ghost.

The project focuses on 3D graphics, camera systems, geometric transformations, lighting, and user interaction.

## Features

* **3D Maze:** Generation of the scene from a matrix representing the walls and navigable areas.
* **Player Movement:** Navigating through the maze while avoiding obstacles.
* **Enemy Behavior:** Automatic movement of the ghost through the maze.
* **Coin Collection:** Collecting coins distributed throughout the scene.
* **Different Camera Views:** Perspective overview, first-person view, and minimap with a top-down view.
* **Interactive camera controls:** rotating the scene and controlling the zoom.
* **3D graphics:** use of models, textures, and geometric transformations.
* **Shaders and lighting:** ambient lighting, spotlights, and vertex and fragment shaders.
* **Night mode:** alternative lighting and a flashlight for the character.
* **Graphical interface:** start and restart the game, coin counter, and victory or end-of-game messages.

## Technologies Used

* **Qt:** graphical interface and user interaction.
* **OpenGL:** rendering and 3D scene management.
* **GLSL:** shader programming and lighting effects.
* **Assimp:** 3D model loading.
* **C++:**

## How to Play

### Objective

Collect the 10 coins scattered throughout the maze and reach the exit without getting caught by the ghost.
Recoge las 10 monedas repartidas por el laberinto y llega a la salida sin que te atrape el fantasma.

### Controls

| Key / Action        | Function                                                                     |
| ------------------- | ---------------------------------------------------------------------------- |
| ⬆️ Up arrow         | Move forward                                                                 |
| ⬅️ Left arrow       | Rotate 90° to the left                                                       |
| ➡️ Right arrow      | Rotate 90° to the right                                                      |
| `C`                 | Toggle between available camera views using this key                         |
| `+` / `-`           | Zoom the camera in or out                                                    |
| Mouse               | Rotate the overview                                                          |
| `N`                 | Turn night mode on or off                                                    |

### Gameplay

1. Click **Start Game** on the interface to begin the game.
2. Move through the maze using the arrow keys.
3. Collect the coins while avoiding the ghost.
4. Collect all 10 coins and reach an exit to win.
5. If the ghost catches you, the game ends.
6. Click **Start Game** again to restart the game.

### Cameras and Lighting

The game includes various viewing options, such as an overview of the maze, a first-person view, and a miniature aerial view. It also allows you to adjust the camera and experiment with lighting and night mode using the controls available in the interface.

## Compiling the Project

To compile and run the project, you need:

* A **Linux** operating system.
* **Qt** installed.
* **qmake**.
* **make**.
* A C++ compiler, such as `g++`.

First, navigate to the directory where the `.pro` file is located.

```bash
cd Laberint
cd entrega
```

Next, run:

```bash
qmake
```
Después, ejecutar:

```bash
make
```

If the compilation completes successfully, the project's executable will be generated.

## Running the Program

Once compiled, run the program using:

```bash
./Laberint
```
## Cleaning Up

To delete the files generated during compilation, use:

```bash
make clean
```

This allows you to recompile the project from scratch if necessary.

## Game Screenshots

### Game Start Screen

![Vista inicial del juego](capturas/Inicio_Juego.png)

### First-Person View

![Vista primera persona](capturas/Vista_Primera_Persona.png)

### Night Mode

![Modo nocturno](capturas/Modo_noche.png)

### Changing the lighting color

![Cambio color](capturas/Cambio_Color_Iluminacion.png)

