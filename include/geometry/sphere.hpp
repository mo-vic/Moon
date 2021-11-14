#ifndef __SPHERE_H_
#define __SPHERE_H_

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <iostream>

// 半径
#define RADIUS 3.0f
// 经纬度粒度
#define LONGITUDE_GRANULARITY 100
#define LATITUDE_GRANULARITY 100


class Sphere
{
private:
    float radius = 0.0f;
    std::vector <float> vertices;
    std::vector <unsigned int> indices;
    unsigned int longitude_granularity;
    unsigned int latitude_granularity;
public:
    Sphere();
    Sphere(float radius, unsigned int longitude_granularity, unsigned int latitude_granularity);
    unsigned int get_longitude_granularity() const;
    unsigned int get_latitude_granularity() const;
    std::vector<float>& get_vertices();
    std::vector<unsigned int>& get_indices();
    
    ~Sphere();
};

Sphere::Sphere()
{
}

Sphere::Sphere(float radius, unsigned int longitude_granularity, unsigned int latitude_granularity)
{
    this->radius = radius;
    this->longitude_granularity = longitude_granularity;
    this->latitude_granularity = latitude_granularity;

    float x, y, z = 0.0f;
    float u, v = 0.0f;
    float length = 0.0f;
    float u_step = 1.0f / (float)(longitude_granularity - 1);
    float v_step = 1.0f / (float)(latitude_granularity - 1);

    float partial_u_x, partial_u_y, partial_u_z;
    float partial_v_x, partial_v_y, partial_v_z;

    // generate uniformly spaced us and vs
    for (unsigned int i = 0; i < latitude_granularity; i++)
    {
        v = (float)i * v_step;
        for (unsigned int j = 0; j < longitude_granularity; j++)
        {
            u = (float) j * u_step;

            x = radius * sin(M_PI * v) * cos(2.0f * M_PI * u);
            y = -radius * cos(M_PI * v);
            z = -radius * sin(M_PI * v) * sin(2.0f * M_PI * u);

            partial_v_x = M_PI * radius * cos(M_PI * v) * cos(2.0f * M_PI * u);
            partial_v_y = M_PI * radius * sin(M_PI * v);
            partial_v_z = -M_PI * radius * cos(M_PI * v) * sin(2.0f * M_PI * u);

            if (v == 0.0f)
            {
                partial_u_x = partial_v_z;
                partial_u_y = partial_v_y;
                partial_u_z = -partial_v_x;
            }
            else if (v == 1.0f)
            {
                partial_u_x = -partial_v_z;
                partial_u_y = partial_v_y;
                partial_u_z = partial_v_x;
            }
            else
            {
                partial_u_x = -2.0f * M_PI * radius * sin(M_PI * v) * sin(2.0f * M_PI * u);
                partial_u_y = 0;
                partial_u_z = -2.0f * M_PI * radius * sin(M_PI * v) * cos(2.0f * M_PI * u);
            }

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(u);
            vertices.push_back(v);
            vertices.push_back(partial_u_x);
            vertices.push_back(partial_u_y);
            vertices.push_back(partial_u_z);
            vertices.push_back(partial_v_x);
            vertices.push_back(partial_v_y);
            vertices.push_back(partial_v_z);
        }
    }

    for (unsigned int i = 1; i < latitude_granularity; i++)
    {
        for (unsigned int j = 0; j < longitude_granularity; j++)
        {
            // generate the bottom triangle indices
            if (i == 1)
            {
                indices.push_back(j);
                indices.push_back(j + longitude_granularity);
                indices.push_back((j + 1) % longitude_granularity + longitude_granularity);
            }
            // generate the top triangle indices
            else if (i == latitude_granularity - 1)
            {
                indices.push_back(j + (latitude_granularity - 2) * longitude_granularity);
                indices.push_back((j + 1) % longitude_granularity + (latitude_granularity - 2) * longitude_granularity);
                indices.push_back(j + (latitude_granularity - 1) * longitude_granularity);
            }
            // generate the middle triangle indices
            else
            {
                indices.push_back(j + (i-1) * longitude_granularity);
                indices.push_back((j+1) % longitude_granularity + (i-1) * longitude_granularity);
                indices.push_back(j + i * longitude_granularity);
                indices.push_back(j + i * longitude_granularity);
                indices.push_back((j + 1) % longitude_granularity + i * longitude_granularity);
                indices.push_back((j + 1) % longitude_granularity + (i-1) * longitude_granularity);
            }
        }
    }
}

unsigned int Sphere::get_longitude_granularity() const
{
    return this->longitude_granularity;
}

unsigned int Sphere::get_latitude_granularity() const
{
    return this->latitude_granularity;
}

std::vector<float>& Sphere::get_vertices()
{
    return vertices;
}

std::vector<unsigned int>& Sphere::get_indices()
{
    return indices;
}

Sphere::~Sphere()
{
}

#endif
