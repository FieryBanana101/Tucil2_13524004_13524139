#include "GUI.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
using namespace std;

static float vec3Length(const Vector3 &v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static Vector3 vec3Normalize(const Vector3 &v) {
    float len = vec3Length(v);
    if (len < 1e-8f) return Vector3(0);
    return v * (1.0f / len);
}


GUI::GUI(Octree *octree)
    : octree(octree),
      cameraYaw(0.4f),
      cameraPitch(0.3f),
      cameraDistance(0.0f),
      cameraTarget(),
      windowWidth(1280),
      windowHeight(720),
      isDragging(false),
      lastMousePos(0, 0)
{
    AABB &rootBox = octree->getRoot()->getBoundingBox();
    cameraTarget = rootBox.getCenter();

    Vector3 size = rootBox.max - rootBox.min;
    float diagonal = vec3Length(size);
    cameraDistance = diagonal * 1.8f;

    collectFaces();
    cout << "Viewer loaded with " << faces.size() << " faces from " << octree->getVoxelNum() << " voxels.\n";
}


void GUI::collectFaces() {
    faces.clear();
    faces.reserve(octree->getVoxelNum() * 6);

    octree->traverse([this](OctreeNode *currNode, int currDepth) {
        (void)currDepth;
        if (currNode->getType() != OCTREE_FILLED_LEAF) return;

        AABB &box = currNode->getBoundingBox();
        Vector3 v0(box.min.x, box.min.y, box.min.z);
        Vector3 v1(box.max.x, box.min.y, box.min.z);
        Vector3 v2(box.max.x, box.max.y, box.min.z);
        Vector3 v3(box.min.x, box.max.y, box.min.z);
        Vector3 v4(box.min.x, box.min.y, box.max.z);
        Vector3 v5(box.max.x, box.min.y, box.max.z);
        Vector3 v6(box.max.x, box.max.y, box.max.z);
        Vector3 v7(box.min.x, box.max.y, box.max.z);

        auto addFace = [this](Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n) {
            Face f;
            f.vertices[0] = a; f.vertices[1] = b;
            f.vertices[2] = c; f.vertices[3] = d;
            f.normal = n;
            f.center = (a + b + c + d) * 0.25f;
            faces.push_back(f);
        };

        // Front  (-Z)
        addFace(v0, v1, v2, v3, Vector3(0, 0, -1));
        // Back   (+Z)
        addFace(v5, v4, v7, v6, Vector3(0, 0, 1));
        // Left   (-X)
        addFace(v4, v0, v3, v7, Vector3(-1, 0, 0));
        // Right  (+X)
        addFace(v1, v5, v6, v2, Vector3(1, 0, 0));
        // Top    (+Y)
        addFace(v3, v2, v6, v7, Vector3(0, 1, 0));
        // Bottom (-Y)
        addFace(v4, v5, v1, v0, Vector3(0, -1, 0));
    });
}


Vector3 GUI::getCameraPosition() const {
    return Vector3(
        cameraTarget.x + cameraDistance * cos(cameraPitch) * sin(cameraYaw),
        cameraTarget.y + cameraDistance * sin(cameraPitch),
        cameraTarget.z + cameraDistance * cos(cameraPitch) * cos(cameraYaw)
    );
}


Vector3 GUI::transformToCamera(const Vector3 &worldPos) const {
    Vector3 camPos = getCameraPosition();
    Vector3 forward = vec3Normalize(cameraTarget - camPos);
    Vector3 worldUp(0, 1, 0);
    Vector3 right = vec3Normalize(forward ^ worldUp);
    Vector3 up = right ^ forward;
    Vector3 rel = worldPos - camPos;
    return Vector3(rel * right, rel * up, rel * forward);
}


Vector3 GUI::project3Dto2D(const Vector3 &worldPos) const {
    Vector3 cam = transformToCamera(worldPos);

    float fov = 60.0f * 3.14159265f / 180.0f;
    float fovFactor = 1.0f / tan(fov / 2.0f);
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    float screenX = (fovFactor * cam.x / (cam.z * aspect)) * (windowWidth / 2.0f) + (windowWidth / 2.0f);
    float screenY = (-fovFactor * cam.y / cam.z) * (windowHeight / 2.0f) + (windowHeight / 2.0f);

    return Vector3(screenX, screenY, cam.z);
}


void GUI::render(sf::RenderWindow &window) {
    Vector3 camPos = getCameraPosition();
    Vector3 lightDir = vec3Normalize(Vector3(0.3f, 0.7f, 0.5f));

    struct FaceRef {
        size_t index;
        float depth;
    };

    vector<FaceRef> visible;
    visible.reserve(faces.size());

    for (size_t i = 0; i < faces.size(); i++) {
        /*
            Back-face culling:
            If the dot product of the face normal and the vector from the camera to the face is positive,
            the face is facing away from the camera and should not be rendered.
            https://ohiostate.pressbooks.pub/app/uploads/sites/45/2017/09/ten-hidden-surface.pdf
        */
        Vector3 toFace = vec3Normalize(faces[i].center - camPos);
        if (vec3Normalize(faces[i].normal) * toFace > 0.0f) continue;

        Vector3 camSpace = transformToCamera(faces[i].center);
        if (camSpace.z < 0.1f) continue;

        visible.push_back({i, camSpace.z});
    }

    /*
        Painter's algorithm: sort farthest first
        We took the algorithm from https://ohiostate.pressbooks.pub/app/uploads/sites/45/2017/09/newell-newell-sancha.pdf
        
    */
    sort(visible.begin(), visible.end(),
              [](const FaceRef &a, const FaceRef &b) { return a.depth > b.depth; });

    for (const auto &ref : visible) {
        const Face &face = faces[ref.index];

        sf::ConvexShape shape(4);
        bool valid = true;

        for (int i = 0; i < 4; i++) {
            Vector3 proj = project3Dto2D(face.vertices[i]);
            if (proj.z < 0.1f) { valid = false; break; }
            shape.setPoint(i, sf::Vector2f(proj.x, proj.y));
        }
        if (!valid) continue;

        // Shading based on face normal direction
        float intensity = max(0.0f, vec3Normalize(face.normal) * lightDir);
        float ambient = 0.25f;
        float brightness = ambient + (1.0f - ambient) * intensity;

        uint8_t r = static_cast<uint8_t>(80 * brightness);
        uint8_t g = static_cast<uint8_t>(200 * brightness);
        uint8_t b = static_cast<uint8_t>(130 * brightness);

        shape.setFillColor(sf::Color(r, g, b));
        shape.setOutlineColor(sf::Color(0, 0, 0, 50));
        shape.setOutlineThickness(0.5f);

        window.draw(shape);
    }
}


void GUI::run() {
    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Tucil 2 - 13524004 & 13524139");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    isDragging = true;
                    lastMousePos = mousePressed->position;
                }
            }

            if (const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseReleased->button == sf::Mouse::Button::Left) {
                    isDragging = false;
                }
            }

            if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (isDragging) {
                    sf::Vector2i delta = mouseMoved->position - lastMousePos;
                    cameraYaw += delta.x * 0.005f;
                    cameraPitch += delta.y * 0.005f;
                    cameraPitch = clamp(cameraPitch, -1.5f, 1.5f);
                    lastMousePos = mouseMoved->position;
                }
            }

            if (const auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                cameraDistance -= scroll->delta * cameraDistance * 0.1f;
                cameraDistance = max(cameraDistance, 0.1f);
            }
        }

        window.clear(sf::Color(30, 30, 30));
        render(window);
        window.display();
    }
}
