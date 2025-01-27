# Robot Simulation in OpenGL

This project is a simulation of a robotic model built entirely in OpenGL using basic primitives, spare cylinders, and shaders. The robot's motion is achieved using kinematics, allowing dynamic interaction via the keyboard. The project includes features like dynamic lighting, ambient and diffuse effects, and additional objects in the scene with various shaders applied. It also incorporates static reflection for the robot.

---

## Features

### Robot Design and Movement
- **Spare Cylinders**: The robot is built entirely from cylinder primitives.
- **Joints**: The robot includes articulated joints such as:
  - Shoulder
  - Elbow
- **Kinematics**: Implemented kinematic equations to allow smooth and realistic movement of the robot.
- **Keyboard Interaction**: Control the robot’s movement and joint rotations via keyboard inputs.
- **Weight Management**: Adjust the movement weights for the joints (elbow and shoulder) using the ImGui library.

### Scene Features
- **Dynamic Lighting**:
  - Ambient and diffuse lighting to enhance realism.
  - Multiple light sources in the scene.
- **Static Reflection**:
  - A reflective surface (e.g., ground plane) that renders a static reflection of the robot for visual aesthetics.

### Additional Objects
- Several objects are present in the scene to provide a context for the robot's environment.
- These objects use different shaders for unique visual effects.

---

## Installation

### Prerequisites
Ensure the following are installed on your system:
- OpenGL (Lower versions)
- C++ Compiler (GCC, MSVC, or Clang)
- GLFW and GLEW libraries
- ImGui library for GUI controls

---

## Controls

### Keyboard Controls
- **Arrow Keys**: Rotate the robot.
- **W / S**: Move the robot forward and backward.
- **A / D**: Rotate the robot left and right.
- **Q / E**: Adjust the shoulder joint.
- **Z / X**: Adjust the elbow joint.

### GUI Controls (ImGui)
- Adjust weights for shoulder and elbow joint movements.
- Modify lighting parameters (e.g., intensity, color, etc.).

---

## Future Enhancements
- Add animations for the robot.
- Introduce more advanced shaders for realistic textures.
- Implement dynamic reflections.
- Extend the scene with additional interactive objects.

---

## Acknowledgments
- **OpenGL** for rendering.
- **GLFW** and **GLEW** for window and context management.
- **ImGui** for the GUI interface.

---

## License
This project is licensed under the MIT License. See the LICENSE file for details.

---

## Contact
For questions, suggestions, or contributions, feel free to reach out:
- Email: [a7vi1912@gmail.com]
- GitHub: [PixelSenseiAvi]

Happy coding!

