#version 330 core
layout (location = 0) in vec3 vertex_coordinates;
layout (location = 1) in vec2 texture_coordinates;
layout (location = 2) in vec3 tangent_vector_u;
layout (location = 3) in vec3 tangent_vector_v;


uniform float radius;
uniform mat4 light_space_matrix;
uniform mat4 model_matrix;
uniform sampler2D displacement_texture;

void main()
{
   float height = texture(displacement_texture, texture_coordinates).x;
   height = ((20504.0f - 1121.5f) * height - 10000.0f) / 1727400.0f * radius;
   vec3 normal = normalize(cross(tangent_vector_u, tangent_vector_v));
   vec4 coordinates = vec4(vertex_coordinates + height * normal, 1);
   gl_Position = light_space_matrix * model_matrix * coordinates;
}
