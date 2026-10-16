#pragma once
#include "Mesh.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct SceneMaterial {
    glm::vec3 color{0.8f,0.8f,0.8f};
    float alpha{1.0f};
    std::string image, opacityImage;
    GLuint texture{0}, opacityTexture{0};
};
class ObjScene {
public:
    bool Load(const std::filesystem::path& objPath, const std::filesystem::path& textureDirectory);
    void Render(GLint colorLoc, GLint useTextureLoc, GLint alphaLoc, GLint opacityLoc, GLint useOpacityLoc);
    ~ObjScene();
    glm::vec3 center{0.f};
    float radius{1.f};
private:
    struct Part { std::unique_ptr<Mesh> mesh; std::string material; };
    std::vector<Part> parts;
    std::map<std::string,SceneMaterial> materials;
};
