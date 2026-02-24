Names: Ishika Aggarwal and Venkata Phani (Sri) Kesiraju

All of the additional scenes that we added can be found in the following folder:
extra_credit_files/assets/scenes

Milestone 2 Extra Credit:
1. Adaptive Termination Criterion: If the accumulated ray weight falls below a certain threshold, the ray tracer stops recursing early to save significant rendering time. This can be tested by adjusting the Threshold slider in the GUI, where a higher threshold will stop the ray tracer sooner, and a Threshold value of 0 will force full recursion.

2. Stochastic (Jittered) Supersampling: Instead of using Regular Grid Sampling which involves us firing rays through the exact center, we added a random offset to the ray's position within each grid cell. This allows the image to have high-frequency noise rather than basic "stair-steps", which leads to better visuals in the images. This can be tested by increasing the Supersamples setting in the GUI.

3. Translucent Emissive Objects ("Radioactive Goop"): The materials can now emit light as rays go through them. It is done along the path length of the ray as it passes through the object by adding the emissive light contribution to the total illumination. This can be tested by rendering a .ray scene that has an object with emissive material as it is automatically handled during refraction.

4. Normal/Bump Mapping: The surface can now have small details and bumps without extra shapes through an additional texture map that is used before the shading is calculated. This can be tested by rendering a .ray scene that includes a material that has a paired texture map and normal map.

5. Portals: Rays that enter through one side are teleported out of the other side. There is also a bounce limit that prevents infinite loops. This can be tested by rendering a .ray scene that includes portals as the ray tracer will automatically detect it and perform accordingly.

6. Creative Scene: We created a custom scene with a magical landscape that has a textured unicorns and floating clouds. There is also complex lighting with realistic shadows and glossy reflections. We also included a portal and many different types of elements. This can be tested by loading the scene into the GUI or through the command line.


Milestone 1:
Extra Credit: We implemented Anti-aliasing with Jittered Sampling. Instead of using Regular Grid Sampling which involves us firing rays through the exact center, we added a random offset to the ray's position within each grid cell. This allows the image to have high-frequency noise rather than basic "stair-steps", which leads to better visuals in the images.

1. Recursive Whitted Style Ray Tracing
- We chose to implement a shadow or secondary ray bias to prevent the ray from intersecting the surface it just originated from.     There is a 0.0001 offset for the normal (reflection) or transmission vector (refraction) for this purpose.
- We chose to implement Total Internal Reflection. The discriminant being checked is negative causing no refractive contribution meaning the ray is absorbed no traced.
2. Triangle-Ray Intersection
- We chose to implement BVH along with the Möller-Trumbore intersection algorithm to allow for faster computing of larger renderings like the trimesh dragon. 
- The BVH splits on the longest axis of the bounding box. A leaf node is considered to be any node with less than 4 faces. 
3. Materials and Light 
- For shading, the full-Whitted style model is used as it  includes emissive, ambient, diffuse, and specular terms. Moreover, standard reflection-based specular calculations are used
- For texture mapping, Bilinear Interpolation is used to reduce "blocky" artifacts when textures are viewed up close
- For shadow attenuation, the function continues to trace through objects if they have a non-zero transmission coefficient, meaning light is attenuated by multiplying the current light color by the material's. This allows color shadows.
- The distance attenuation multiplier is set to a maximum of 1.0 to prevent extremely bright spots