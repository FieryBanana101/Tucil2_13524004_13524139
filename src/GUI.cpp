#include "GUI.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>
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
      lastMousePos(0, 0),
      font(),
      fontLoaded(false),
      sceneBounds()
{
    AABB &rootBox = octree->getRoot()->getBoundingBox();
    cameraTarget = rootBox.getCenter();
    sceneBounds = rootBox;

    Vector3 size = rootBox.max - rootBox.min;
    float diagonal = vec3Length(size);
    cameraDistance = diagonal * 1.8f;

#ifdef _WIN32
    fontLoaded = font.openFromFile("C:/Windows/Fonts/consola.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("C:/Windows/Fonts/cour.ttf");
#else
    fontLoaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("/usr/share/fonts/truetype/freefont/FreeMono.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("/mnt/c/Windows/Fonts/consola.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("/mnt/c/Windows/Fonts/arial.ttf");
#endif

    collectFaces();

    if (!faces.empty()) {
        Vector3 tightMin(1e18f), tightMax(-1e18f);
        for (const auto &f : faces) {
            for (int i = 0; i < 4; i++) {
                tightMin.x = min(tightMin.x, f.vertices[i].x);
                tightMin.y = min(tightMin.y, f.vertices[i].y);
                tightMin.z = min(tightMin.z, f.vertices[i].z);
                tightMax.x = max(tightMax.x, f.vertices[i].x);
                tightMax.y = max(tightMax.y, f.vertices[i].y);
                tightMax.z = max(tightMax.z, f.vertices[i].z);
            }
        }
        sceneBounds = AABB(tightMin, tightMax);
        cameraTarget = sceneBounds.getCenter();
        Vector3 tightSize = tightMax - tightMin;
        cameraDistance = vec3Length(tightSize) * 1.8f;
    }

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


void GUI::drawGrid(sf::RenderWindow &window) {
    float y = sceneBounds.min.y;
    Vector3 size = sceneBounds.max - sceneBounds.min;
    float extend = 0.5f;
    float gxMin = sceneBounds.min.x - extend * size.x;
    float gxMax = sceneBounds.max.x + extend * size.x;
    float gzMin = sceneBounds.min.z - extend * size.z;
    float gzMax = sceneBounds.max.z + extend * size.z;

    int gridLines = 12;
    float xStep = (gxMax - gxMin) / gridLines;
    float zStep = (gzMax - gzMin) / gridLines;
    sf::Color gridColor(255, 255, 255, 60);

    for (int i = 0; i <= gridLines; i++) {
        float x = gxMin + i * xStep;
        Vector3 s0 = project3Dto2D(Vector3(x, y, gzMin));
        Vector3 s1 = project3Dto2D(Vector3(x, y, gzMax));
        if (s0.z < 0.1f || s1.z < 0.1f) continue;
        sf::Vertex line[2] = {
            {sf::Vector2f(s0.x, s0.y), gridColor},
            {sf::Vector2f(s1.x, s1.y), gridColor}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    for (int i = 0; i <= gridLines; i++) {
        float z = gzMin + i * zStep;
        Vector3 s0 = project3Dto2D(Vector3(gxMin, y, z));
        Vector3 s1 = project3Dto2D(Vector3(gxMax, y, z));
        if (s0.z < 0.1f || s1.z < 0.1f) continue;
        sf::Vertex line[2] = {
            {sf::Vector2f(s0.x, s0.y), gridColor},
            {sf::Vector2f(s1.x, s1.y), gridColor}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}


void GUI::drawOriginCross(sf::RenderWindow &window) {
    float y = sceneBounds.min.y;
    Vector3 size = sceneBounds.max - sceneBounds.min;
    float extend = 0.5f;
    float gxMin = sceneBounds.min.x - extend * size.x;
    float gxMax = sceneBounds.max.x + extend * size.x;
    float gyMin = sceneBounds.min.y - extend * size.y;
    float gyMax = sceneBounds.max.y + extend * size.y;
    float gzMin = sceneBounds.min.z - extend * size.z;
    float gzMax = sceneBounds.max.z + extend * size.z;
    float cx = cameraTarget.x, cz = cameraTarget.z;
    
    sf::Color colorX(0, 220, 0, 200);   // Green
    sf::Color colorY(60, 60, 255, 200); // Blue
    sf::Color colorZ(220, 0, 0, 200);   // Red

    Vector3 sx0 = project3Dto2D(Vector3(gxMin, y, cz));
    Vector3 sx1 = project3Dto2D(Vector3(gxMax, y, cz));
    if (sx0.z > 0.1f && sx1.z > 0.1f) {
        sf::Vertex xLine[2] = {
            {sf::Vector2f(sx0.x, sx0.y), colorX},
            {sf::Vector2f(sx1.x, sx1.y), colorX}
        };
        window.draw(xLine, 2, sf::PrimitiveType::Lines);
    }

    Vector3 sy0 = project3Dto2D(Vector3(cx, gyMin, cz));
    Vector3 sy1 = project3Dto2D(Vector3(cx, gyMax, cz));
    if (sy0.z > 0.1f && sy1.z > 0.1f) {
        sf::Vertex yLine[2] = {
            {sf::Vector2f(sy0.x, sy0.y), colorY},
            {sf::Vector2f(sy1.x, sy1.y), colorY}
        };
        window.draw(yLine, 2, sf::PrimitiveType::Lines);
    }

    Vector3 sz0 = project3Dto2D(Vector3(cx, y, gzMin));
    Vector3 sz1 = project3Dto2D(Vector3(cx, y, gzMax));
    if (sz0.z > 0.1f && sz1.z > 0.1f) {
        sf::Vertex zLine[2] = {
            {sf::Vector2f(sz0.x, sz0.y), colorZ},
            {sf::Vector2f(sz1.x, sz1.y), colorZ}
        };
        window.draw(zLine, 2, sf::PrimitiveType::Lines);
    }
}


void GUI::drawAxisLegend(sf::RenderWindow &window) {
    Vector3 camPos = getCameraPosition();
    Vector3 forward = vec3Normalize(cameraTarget - camPos);
    Vector3 worldUp(0, 1, 0);
    Vector3 right = vec3Normalize(forward ^ worldUp);
    Vector3 up = right ^ forward;

    sf::Vector2f legendCenter(windowWidth - 90.f, 90.f);
    float axisLen = 50.f;
    sf::RectangleShape panel(sf::Vector2f(120.f, 120.f));
    panel.setFillColor(sf::Color(0, 0, 0, 130));
    panel.setPosition(sf::Vector2f(windowWidth - 150.f, 30.f));
    window.draw(panel);

    auto projectAxis = [&](Vector3 axis) -> sf::Vector2f {
        return sf::Vector2f((axis * right) * axisLen, -(axis * up) * axisLen);
    };

    sf::Vector2f xDir = projectAxis(Vector3(1, 0, 0));
    sf::Vector2f yDir = projectAxis(Vector3(0, 1, 0));
    sf::Vector2f zDir = projectAxis(Vector3(0, 0, 1));

    struct AxisDef { sf::Vector2f dir; sf::Color color; const char *label; };
    AxisDef axes[3] = {
        { xDir, sf::Color(0, 220, 0),   "X" },
        { yDir, sf::Color(60, 60, 255),  "Y" },
        { zDir, sf::Color(220, 0, 0),   "Z" },
    };

    for (auto &ax : axes) {
        sf::Vertex line[2] = {
            {legendCenter, ax.color},
            {legendCenter + ax.dir, ax.color}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);

        if (fontLoaded) {
            sf::Text label(font, ax.label, 14u);
            label.setFillColor(ax.color);
            label.setPosition(legendCenter + ax.dir + sf::Vector2f(-5.f, -8.f));
            window.draw(label);
        }
    }

    sf::CircleShape dot(3.f);
    dot.setFillColor(sf::Color::White);
    dot.setPosition(legendCenter - sf::Vector2f(3.f, 3.f));
    window.draw(dot);
}


void GUI::drawOverlays(sf::RenderWindow &window) {
    if (!fontLoaded) return;

    Vector3 camPos = getCameraPosition();
    Vector3 forward = vec3Normalize(cameraTarget - camPos);

    char posStr[128], dirStr[128];
    snprintf(posStr, sizeof(posStr), "Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
    snprintf(dirStr, sizeof(dirStr), "Direction: (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);

    sf::Text posText(font, posStr, 15u);
    posText.setFillColor(sf::Color::Yellow);
    posText.setPosition(sf::Vector2f(10.f, 10.f));

    sf::Text dirText(font, dirStr, 15u);
    dirText.setFillColor(sf::Color::Yellow);
    dirText.setPosition(sf::Vector2f(10.f, 32.f));

    window.draw(posText);
    window.draw(dirText);
}


void GUI::render(sf::RenderWindow &window) {
    drawGrid(window);
    drawOriginCross(window);

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

        uint8_t r = static_cast<uint8_t>(180 * brightness);
        uint8_t g = static_cast<uint8_t>(180 * brightness);
        uint8_t b = static_cast<uint8_t>(180 * brightness);

        shape.setFillColor(sf::Color(r, g, b));
        shape.setOutlineColor(sf::Color(0, 0, 0, 50));
        shape.setOutlineThickness(0.5f);

        window.draw(shape);
    }

    drawAxisLegend(window);
    drawOverlays(window);
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
                    cameraYaw -= delta.x * 0.005f;
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

        window.clear(sf::Color::Black);
        render(window);
        window.display();
    }
}
