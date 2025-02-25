//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_GLHELPER_H
#define LIMONENGINE_GLHELPER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <algorithm>
#include <vector>

#include <fstream>
#include <streambuf>
#include <iostream>
#include <unordered_map>
#include <GL/glew.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#  include <GL/gl.h>
#include <memory>

#endif/*__APPLE__*/

#define NR_POINT_LIGHTS 3
#define NR_TOTAL_LIGHTS 4
#define NR_MAX_MODELS (1000)
#define NR_MAX_MATERIALS 2000

#include "Options.h"
class Material;

class Light;

class GLSLProgram;

struct Line {
    glm::vec3 from;
    glm::vec3 fromColor;
    int needsCameraTransform;//FIXME This variable is repeated, because it must be per vertex. Maybe we can share int as bytes.
    glm::vec3 to;
    glm::vec3 toColor;
    int needsCameraTransform2;//this is the other one

    Line(const glm::vec3 &from,
         const glm::vec3 &to,
         const glm::vec3 &fromColor,
         const glm::vec3 &toColor,
         const bool needsCameraTransform): from(from), fromColor(fromColor), needsCameraTransform(needsCameraTransform), to(to), toColor(toColor), needsCameraTransform2(needsCameraTransform){};
};

class GLHelper {
    class OpenglState {
        unsigned int activeProgram;
        unsigned int activeTextureUnit;
        std::vector<unsigned int> textures;

        void attachTexture(GLuint textureID, GLuint textureUnit, GLenum type) {
            if (textures[textureUnit] != textureID) {
                textures[textureUnit] = textureID;
                activateTextureUnit(textureUnit);
                glBindTexture(type, textureID);
            }
        }

    public:
        uint32_t programChangeCount=0;

        explicit OpenglState(GLint textureUnitCount) : activeProgram(0) {
            textures.resize(textureUnitCount);
            memset(textures.data(), 0, textureUnitCount * sizeof(int));
            activeTextureUnit = 0;
            glActiveTexture(GL_TEXTURE0);
        }

        void activateTextureUnit(GLuint textureUnit) {
            if (activeTextureUnit != textureUnit) {
                activeTextureUnit = textureUnit;
                //https://www.khronos.org/opengles/sdk/1.1/docs/man/glActiveTexture.xml guarantees below works for texture selection
                glActiveTexture(GL_TEXTURE0 + textureUnit);
            }
        }

        void attachTexture(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_2D);
        }

        void attach2DTextureArray(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_2D_ARRAY);
        }

        void attachCubemap(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_CUBE_MAP);
        }

        bool deleteTexture(GLuint textureID) {
            //clear this textures attachment state
            for (size_t i = 0; i < textures.size(); ++i) {
                if(textures[i] == textureID) {
                    textures[i] = 0;
                }
            }
            if (glIsTexture(textureID)) {
                glDeleteTextures(1, &textureID);
                return true;
            } else {

                return false;
            }
        }

        void attachCubemapArray(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_CUBE_MAP_ARRAY_ARB);
        }


        void setProgram(GLuint program) {
            if (program != this->activeProgram) {
                programChangeCount++;
                glUseProgram(program);
                this->activeProgram = program;
            }
        }
    };


public:
    enum VariableTypes {
        INT,
        FLOAT,
        FLOAT_VEC2,
        FLOAT_VEC3,
        FLOAT_VEC4,
        FLOAT_MAT4,
        UNDEFINED };


    class Uniform{
    public:
        unsigned int location;
        std::string name;
        VariableTypes type;
        unsigned int size;

        Uniform(unsigned int location, const std::string &name, GLenum typeEnum, unsigned int size) : location(
                location), name(name), size(size) {
            switch (typeEnum) {
                case GL_SAMPLER_CUBE: //these are because sampler takes a int as texture unit
                case GL_SAMPLER_CUBE_MAP_ARRAY_ARB:
                case GL_SAMPLER_2D:
                case GL_SAMPLER_2D_ARRAY:
                case GL_INT:
                    type = INT;
                    break;
                case GL_FLOAT:
                    type = FLOAT;
                    break;
                case GL_FLOAT_VEC2:
                    type = FLOAT_VEC2;
                    break;
                case GL_FLOAT_VEC3:
                    type = FLOAT_VEC3;
                    break;
                case GL_FLOAT_VEC4:
                    type = FLOAT_VEC4;
                    break;
                case GL_FLOAT_MAT4:
                    type = FLOAT_MAT4;
                    break;
                default:
                    type = UNDEFINED;
            }
        }
    };

    enum FrustumSide
    {
        RIGHT	= 0,		// The RIGHT side of the frustum
        LEFT	= 1,		// The LEFT	 side of the frustum
        BOTTOM	= 2,		// The BOTTOM side of the frustum
        TOP		= 3,		// The TOP side of the frustum
        BACK	= 4,		// The BACK	side of the frustum
        FRONT	= 5			// The FRONT side of the frustum
    };

private:
    GLenum error;
    uint32_t nextMaterialIndex = 0;//this is used to keep each material in the  GPU memory. imagine it like size of vector
    GLint maxTextureImageUnits;
    OpenglState *state;

    unsigned int screenHeight, screenWidth;
    float aspect;
    std::vector<GLuint> bufferObjects;
    std::vector<GLuint> vertexArrays;
    std::vector<GLuint> modelIndexesTemp;


    GLuint lightUBOLocation;
    GLuint playerUBOLocation;
    GLuint allMaterialsUBOLocation;
    GLuint allModelsUBOLocation;
    GLuint allModelIndexesUBOLocation;

    uint32_t activeMaterialIndex;

    GLuint depthOnlyFrameBufferDirectional;
    GLuint depthMapDirectional;

    GLuint depthOnlyFrameBufferPoint;
    GLuint depthCubemapPoint;


    GLuint coloringFrameBuffer;
    GLuint normalMap;
    GLuint diffuseAndSpecularLightedMap;
    GLuint ambientMap;
    GLuint depthMap;

    GLuint ssaoGenerationFrameBuffer;
    GLuint ssaoMap;

    GLuint ssaoBlurFrameBuffer;
    GLuint ssaoBlurredMap;

    GLuint combineFrameBuffer;

    unsigned int noiseTexture;

    Options *options;

    const uint_fast32_t lightUniformSize = (sizeof(glm::mat4) * 7) + (4 * sizeof(glm::vec4));
    const uint32_t playerUniformSize = 5 * sizeof(glm::mat4)+ 3* sizeof(glm::vec4);
    int32_t materialUniformSize = 2 * sizeof(glm::vec3) + sizeof(float) + sizeof(GLuint);
    int32_t modelUniformSize = sizeof(glm::mat4);

    glm::mat4 cameraMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    glm::mat4 inverseProjection;
    std::vector<glm::vec4>frustumPlanes;
    glm::mat4 orthogonalProjectionMatrix;
    glm::mat4 lightProjectionMatrixDirectional;
    glm::mat4 lightProjectionMatrixPoint;
    glm::vec3 cameraPosition;
    uint32_t renderTriangleCount;
    uint32_t renderLineCount;
    uint32_t uniformSetCount=0;


public:
    void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) {
        triangleCount = renderTriangleCount;
        lineCount = renderLineCount;
    }

    const glm::mat4 &getLightProjectionMatrixPoint() const {
        return lightProjectionMatrixPoint;
    }

    const glm::mat4 &getLightProjectionMatrixDirectional() const {
        return lightProjectionMatrixDirectional;
    }

private:
    inline bool checkErrors(const std::string &callerFunc [[gnu::unused]]) {
#ifndef NDEBUG
        GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "FB status while " << callerFunc << " is " << fbStatus << std::endl;
        }
        bool hasError = false;
        while ((error = glGetError()) != GL_NO_ERROR) {
            std::cerr << "error found on GL context while " << callerFunc << ":" << error << ":" << gluErrorString(error)
                      << std::endl;
            hasError = true;
        }
        return hasError;
#else
        return false;
#endif
    };

    GLuint createShader(GLenum, const std::string &);

    GLuint createProgram(const std::vector<GLuint> &);

    GLuint generateBuffer(const GLuint number);

    bool deleteBuffer(const GLuint number, const GLuint bufferID);

    GLuint generateVAO(const GLuint number);

    bool deleteVAO(const GLuint number, const GLuint bufferID);

    void fillUniformMap(const GLuint program, std::unordered_map<std::string, Uniform *> &uniformMap) const;

    void attachGeneralUBOs(const GLuint program);
    void bufferExtraVertexData(uint_fast32_t elementPerVertexCount, GLenum elementType, uint_fast32_t dataSize,
                               const void *extraData, uint_fast32_t &vao, uint_fast32_t &vbo,
                               const uint_fast32_t attachPointer);

public:
    explicit GLHelper(Options *options);

    ~GLHelper();

    void attachModelUBO(const uint32_t program);

    void attachMaterialUBO(const uint32_t program, const uint32_t materialID);

    uint32_t getNextMaterialIndex() {
        return nextMaterialIndex++;
    }

    GLuint initializeProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile,
                                 std::unordered_map<std::string, Uniform *> &);

    void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer, uint_fast32_t &ebo);

    void bufferNormalData(const std::vector<glm::vec3> &colors,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer);

    void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer);

    void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer);

    void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                        uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer);

    bool freeBuffer(const GLuint bufferID);

    bool freeVAO(const GLuint VAO);

    void clearFrame() {

        //additional depths for Directional is not needed, but depth for point is reqired, because there is no way to clear
        //it per layer, so we are clearing per frame. This also means, lights should not reuse the textures.
        glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferPoint);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferDirectional);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, coloringFrameBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoGenerationFrameBuffer);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFrameBuffer);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);//combining doesn't need depth test either
        glClear(GL_COLOR_BUFFER_BIT);//clear for default

        renderTriangleCount = 0;
        renderLineCount = 0;
        //std::cout << "program change count was : " << state->programChangeCount << std::endl;
        state->programChangeCount = 0;

        //std::cout << "uniform set count was : " << uniformSetCount << std::endl;
        uniformSetCount = 0;
    }

    void render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount);

    void reshape();

    GLuint loadTexture(int height, int width, GLenum format, void *data);

    GLuint loadCubeMap(int height, int width, void *right, void *left, void *top, void *bottom, void *back,
                       void *front);

    void attachTexture(unsigned int textureID, unsigned int attachPoint);

    void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint);

    bool deleteTexture(GLuint textureID);

    bool getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location);

    const glm::mat4& getCameraMatrix() const { return cameraMatrix; };

    const glm::vec3& getCameraPosition() const { return cameraPosition; };

    const glm::mat4& getProjectionMatrix() const { return perspectiveProjectionMatrix; };

    const glm::mat4& getOrthogonalProjectionMatrix() const { return orthogonalProjectionMatrix; }

    void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize);

    void drawLines(GLSLProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines);

    void clearDepthBuffer() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix);

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector);

    bool setUniform(const GLuint programID, const GLuint uniformID, const std::vector<glm::vec3> &vectorArray);

    bool setUniform(const GLuint programID, const GLuint uniformID, const float value);

    bool setUniform(const GLuint programID, const GLuint uniformID, const int value);

    bool setUniformArray(const GLuint programID, const GLuint uniformID, const std::vector<glm::mat4> &matrixArray);

    void setLight(const Light &light, const int i);

    void removeLight(const int i) {
        GLint temp = 0;
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4) + sizeof(glm::vec3),
                        sizeof(GLint), &temp);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        checkErrors("removeLight");
    }

    void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix);

    void switchRenderToShadowMapDirectional(const unsigned int index);
    void switchRenderToShadowMapPoint();
    void switchRenderToColoring();
    void switchRenderToSSAOGeneration();
    void switchRenderToSSAOBlur();
    void switchRenderToCombining();

    int getMaxTextureImageUnits() const {
        return maxTextureImageUnits;
    }

    void calculateFrustumPlanes(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix,
                                std::vector<glm::vec4> &planes) const;

    inline bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
        return isInFrustum(aabbMin, aabbMax, frustumPlanes);
    }

    inline bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax, const std::vector<glm::vec4>& frustumPlaneVector) const {
        bool inside = true;
        //test all 6 frustum planes
        for (int i = 0; i<6; i++) {
            //pick closest point to plane and check if it behind the plane
            //if yes - object outside frustum
            float d =   std::fmax(aabbMin.x * frustumPlaneVector[i].x, aabbMax.x * frustumPlaneVector[i].x)
                      + std::fmax(aabbMin.y * frustumPlaneVector[i].y, aabbMax.y * frustumPlaneVector[i].y)
                      + std::fmax(aabbMin.z * frustumPlaneVector[i].z, aabbMax.z * frustumPlaneVector[i].z)
                      + frustumPlaneVector[i].w;
            inside &= d > 0;
            //return false; //with flag works faster
        }
        return inside;
    }

    void setMaterial(std::shared_ptr<const Material>material);

    void setModel(const uint32_t modelID, const glm::mat4 &worldTransform);

    void setModelIndexesUBO(std::vector<uint32_t> &modelIndicesList);

    void attachModelIndicesUBO(const uint32_t programID);

    void renderInstanced(GLuint program, uint_fast32_t VAO, uint_fast32_t EBO, uint_fast32_t triangleCount,
                         uint32_t instanceCount);
};

#endif //LIMONENGINE_GLHELPER_H
