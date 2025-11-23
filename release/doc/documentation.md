## Controls

WASD - Camera movement

Arrow Keys - Camera rotation

## Development Status

We are using Vulkan without any framework, as we described in the Submission 1: Task Description.

- Right now most of the core Vulkan part is implemented.

- We implemented a `render system` that users can edit the rendering pipeline configurations as they want. Right now we are using a default pipeline configured for solid, opaque, double-sided triangles with depth test enabled, and viewport size is adjustable.

- We implemented a `camera` class which adjusts the projection matrices. And it controls the camera movement along with a input management class.

- We implemented a `game object` class which has a transform component inside, and calculates the transformation calculations.

- We are using a left handed and -y up coordinate system.

- We implemented a `mesh` class - TODO

- We implemented a `model` class - TODO

- We are using `assimp` as object loader.

- We can assing textures to objects, and it can be rendered.

- We implemented a `lightmanager` class that handles the position and instensity values of a number of `point` lights and one `directional` light.

- We implemented `diffuse shading` for now.

- No complext effect is implemented yet.
