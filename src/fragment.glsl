#version 330 core

in vec3 tan_vec_u;
in vec3 tan_vec_v;
in vec2 texture_coordinates;
in vec3 fragment_coordinates_in_camera_space;
in vec4 fragment_coordinates_in_light_space;

out vec4 FragColor;

uniform float radius;
uniform vec3 view_position;
uniform sampler2D color_texture;
uniform sampler2D displacement_texture;
uniform vec3 light_position;
uniform mat4 model_matrix;
uniform sampler2D shadow_map;
uniform mat4 light_space_matrix;


void main()
{
   float delta = 0.001;
   vec2 delta_u1 = vec2(texture_coordinates.x - delta, texture_coordinates.y);
   vec2 delta_u2 = vec2(texture_coordinates.x + delta, texture_coordinates.y);
   float u_diff = texture(displacement_texture, delta_u2).x - texture(displacement_texture, delta_u1).x;
   u_diff = (20504.0f - 1121.5f) * u_diff / 1727400.0f * radius;
   float d_u = u_diff / (2.0f * delta);

   vec2 delta_v1 = vec2(texture_coordinates.x, texture_coordinates.y - delta);
   vec2 delta_v2 = vec2(texture_coordinates.x, texture_coordinates.y + delta);
   float v_diff = texture(displacement_texture, delta_v2).x - texture(displacement_texture, delta_v1).x;
   v_diff = (20504.0f - 1121.5f) * v_diff / 1727400.0f * radius;
   float d_v = (v_diff) / (2.0f * delta);

   float d_u_v = texture(displacement_texture, texture_coordinates).x;
   d_u_v = (20504.0f - 1121.5f) * d_u_v / 1727400.0f * radius;

   vec3 norm = normalize(cross(tan_vec_u, tan_vec_v));

   vec3 tangent_vector_u = tan_vec_u * (1.0f + 1.0f / 3.0f * d_u_v) + norm * d_u;
   vec3 tangent_vector_v = tan_vec_v * (1.0f + 1.0f / 3.0f * d_u_v) + norm * d_v;

   vec3 normal = normalize(cross(tangent_vector_u, tangent_vector_v));
   normal = mat3(model_matrix) * normal;

   vec3 light_direction = normalize(light_position - fragment_coordinates_in_camera_space);
   float diffuse = max(dot(normal, light_direction), 0.0f);

   vec3 reflect_direction = reflect(-light_direction, normal);
   vec3 view_direction = normalize(view_position - fragment_coordinates_in_camera_space);
   float specular = pow(max(dot(view_direction, reflect_direction), 0.0), 4);
   float specular_strength = 0.01;

   vec3 projected_coordinates = fragment_coordinates_in_light_space.xyz / fragment_coordinates_in_light_space.w;
   projected_coordinates = projected_coordinates * 0.5 + 0.5;
   float cloest_depth = texture(shadow_map, projected_coordinates.xy).r;

   float shadow = 0.0;
   vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
   for (int x = -1; x <= 1; ++x)
   {
      for (int y = -1; y <= 1; ++y)
      {
         float pcd_depth = texture(shadow_map, projected_coordinates.xy + vec2(x, y) * texel_size).r;
         shadow += projected_coordinates.z - 0.0005 > pcd_depth ? 0.0 : 1.0;
      }
   }
   shadow /= 9;


   FragColor = vec4(vec3(texture(color_texture, texture_coordinates)) * (shadow * (specular_strength * specular + diffuse) + 0.015) * vec3(1.0, 1.0, 1.0f), 1.0);
}
