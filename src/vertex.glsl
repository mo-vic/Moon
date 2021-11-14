#version 330 core
layout (location = 0) in vec3 vertex_coordinates;
layout (location = 1) in vec2 vertex_texture_coordinates;
layout (location = 2) in vec3 tangent_vector_u;
layout (location = 3) in vec3 tangent_vector_v;

out vec3 tan_vec_u;
out vec3 tan_vec_v;
out vec2 texture_coordinates;
out vec4 fragment_coordinates_in_light_space;
out vec3 fragment_coordinates_in_camera_space;

uniform float radius;
uniform mat4 model_matrix;
uniform mat4 transform_matrix;
uniform mat4 light_space_matrix;

uniform sampler2D displacement_texture;
uniform sampler2D shadow_map;

void main()
{
   float height = texture(displacement_texture, vertex_texture_coordinates).x;
   height = ((20504.0f - 1121.5f) * height - 10000.0f) / 1727400.0f * radius;

   vec3 normal = normalize(cross(tangent_vector_u, tangent_vector_v));
   vec4 coordinates = vec4(vertex_coordinates + height * normal, 1);

   gl_Position = transform_matrix * coordinates;
   texture_coordinates = vertex_texture_coordinates;
   fragment_coordinates_in_camera_space = vec3(model_matrix * coordinates);
   fragment_coordinates_in_light_space = light_space_matrix * model_matrix * coordinates;
   tan_vec_u = tangent_vector_u;
   tan_vec_v = tangent_vector_v;
}
