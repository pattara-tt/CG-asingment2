#undef GLFW_DLL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Libs/ObjScene.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"

const auto shaderDirectory = std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);
const auto modelDirectory = shaderDirectory.parent_path() / "Models";
const auto textureDirectory = shaderDirectory.parent_path() / "Textures";

int main() {
    Window window(800, 600, 3, 3);
    if (window.initialise() != 0) return 1;
    {
        ObjScene scene;
        if (!scene.Load(modelDirectory / "modelCG1.obj", textureDirectory)) {
            std::cerr << "Failed to load Ghost Kitty scene.\n";
            return 1;
        }

        Shader shader;
        shader.CreateFromFiles(shaderDirectory / "shader.vert",shaderDirectory / "shader.frag");

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        const glm::vec3 cameraPos(0.55f, 1.0f, 2.0f);
        const glm::vec3 cameraTarget(0.0f, 0.8f, 0.0f);
        const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

        const glm::vec3 spotPos(0.2f, 3.2922f, 0.59559f);

        const glm::vec3 spotDirection = glm::normalize(
            glm::vec3(-0.00833f, -0.98770f, -0.15615f)
        );

        // Approximate Blender's warm color; adjust RGB to match your actual color.
        const glm::vec3 spotColor(1.0f, 0.82f, 0.65f);
        const float spotIntensity = 1.0f;  // OpenGL scale, not Blender watts
        const float spotAngle = 55.8f;     // Full cone angle, degrees
        const float spotBlend = 0.780f;    // Soft edge (0=hard, 1=very soft)

        // Approximation of light falloff; not Blender's 0.387m emitter radius.
        const float attenuationConstant = 1.0f;
        const float attenuationLinear = 0.09f;
        const float attenuationQuadratic = 0.032f;
        const float ambientStrength = 0.40f;

        // ========== 3 ADDITIONAL BLENDER POINT LIGHTS ==========
        // OBJ Y-up conversion: Blender (X,Y,Z) -> OpenGL (X,Z,-Y).
        // Blender Power (watts) cannot be converted 1:1 to this shader.
        const glm::vec3 pointPositions[3] = {
            {-0.71520f, 0.36278f, 0.43090f},
            {-0.52279f, 0.50884f, 0.40119f},
            { 0.94005f, 0.37481f, 0.61561f}
        };
        // Approximate colors based on Blender UI swatches (not exact RGB values).
        const glm::vec3 pointColors[3] = {
            {0.45f, 0.34f, 0.07f}, // Dark warm yellow
            {1.00f, 0.78f, 0.16f}, // Bright yellow
            {0.56f, 0.43f, 0.17f}  // Muted gold
        };
        const float pointPowers[3] = {3.0f, 3.0f, 2.0f};
        const float pointIntensityScale = 0.18f; // Adjust all 3 together

        while (!window.getShouldClose()) {
            glfwPollEvents();
            if (glfwGetKey(window.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window.getWindow(), true);

            const glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
            const glm::mat4 projection = glm::perspective(
                glm::radians(80.0f),
                float(window.getBufferWidth()) / float(window.getBufferHeight()),
                0.05f, 1000.0f
            );
            const glm::mat4 model(1.0f);

            glViewport(0, 0, window.getBufferWidth(), window.getBufferHeight());
            glClearColor(0.37f, 0.35f, 0.27f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.UseShader();
            glUniformMatrix4fv(shader.GetUniformLocation("model"), 1, GL_FALSE,
                               glm::value_ptr(model));
            glUniformMatrix4fv(shader.GetUniformLocation("view"), 1, GL_FALSE,
                               glm::value_ptr(view));
            glUniformMatrix4fv(shader.GetUniformLocation("projection"), 1, GL_FALSE,
                               glm::value_ptr(projection));

            const glm::vec3 finalLightColor = spotColor * spotIntensity;
            glUniform3fv(shader.GetUniformLocation("lightColour"), 1,
                         glm::value_ptr(finalLightColor));
            glUniform3fv(shader.GetUniformLocation("lightPos"), 1,
                         glm::value_ptr(spotPos));
            glUniform3fv(shader.GetUniformLocation("spotDirection"), 1,
                         glm::value_ptr(spotDirection));
            glUniform1f(shader.GetUniformLocation("spotAngle"), spotAngle);
            glUniform1f(shader.GetUniformLocation("spotBlend"), spotBlend);
            glUniform1f(shader.GetUniformLocation("ambientStrength"), ambientStrength);
            glUniform3f(shader.GetUniformLocation("attenuation"),
                        attenuationConstant, attenuationLinear, attenuationQuadratic);
            // Send three point lights to fragment shader.
            for (int i = 0; i < 3; ++i) {
                const std::string index = std::to_string(i);
                const std::string posName = "pointLightPos[" + index + "]";
                const std::string colorName = "pointLightColor[" + index + "]";
                const std::string powerName = "pointLightPower[" + index + "]";
                glUniform3fv(shader.GetUniformLocation(posName.c_str()), 1,
                             glm::value_ptr(pointPositions[i]));
                glUniform3fv(shader.GetUniformLocation(colorName.c_str()), 1,
                             glm::value_ptr(pointColors[i]));
                glUniform1f(shader.GetUniformLocation(powerName.c_str()),
                            pointPowers[i] * pointIntensityScale);
            }
            glUniform3fv(shader.GetUniformLocation("viewPos"), 1,
                         glm::value_ptr(cameraPos));
            glUniform1i(shader.GetUniformLocation("texture2D"), 0);

            scene.Render(
                shader.GetUniformLocation("materialColor"),
                shader.GetUniformLocation("hasDiffuseMap"),
                shader.GetUniformLocation("materialAlpha"),
                shader.GetUniformLocation("opacityTexture"),
                shader.GetUniformLocation("hasOpacityMap")
            );
            glUseProgram(0);
            window.swapBuffers();
        }
    }
    return 0;
}
