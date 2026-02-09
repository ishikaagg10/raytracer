Names: Ishika Aggarwal and Venkata Phani (Sri) Kesiraju

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