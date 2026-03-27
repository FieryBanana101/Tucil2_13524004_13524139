#pragma once

#include "Octree.hpp"
#include "AABB.hpp"
#include "Vector3.hpp"
#include <SFML/Graphics.hpp>

#include <vector>

class GUI {
public:
    GUI(Octree *octree);
    void run();

private:
    struct Face {
        Vector3 vertices[4];
        Vector3 normal;
        Vector3 center;
    };

    Octree *octree;
    std::vector<Face> faces;
    float cameraYaw;
    float cameraPitch;
    float cameraDistance;
    Vector3 cameraTarget;
    unsigned int windowWidth;
    unsigned int windowHeight;
    bool isDragging;
    sf::Vector2i lastMousePos;
    sf::Font font;
    bool fontLoaded;
    AABB sceneBounds;

    void collectFaces();
    void render(sf::RenderWindow &window);
    void drawGrid(sf::RenderWindow &window);
    void drawOriginCross(sf::RenderWindow &window);
    void drawAxisLegend(sf::RenderWindow &window);
    void drawOverlays(sf::RenderWindow &window);
    Vector3 getCameraPosition() const;
    Vector3 transformToCamera(const Vector3 &worldPos) const;
    Vector3 project3Dto2D(const Vector3 &worldPos) const;
};
