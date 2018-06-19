//
// Created by Engin Manap on 13.02.2016.
//

#ifndef LIMONENGINE_WORLD_H
#define LIMONENGINE_WORLD_H

#include <vector>
#include <tinyxml2.h>
#include <unordered_map>
#include <set>
#include "glm/glm.hpp"
#include "InputHandler.h"
#include "FontManager.h"
#include "GameObjects/SkyBox.h"
#include "GamePlay/LimonAPI.h"
#include "AI/Actor.h"



class btGhostPairCallback;
class Camera;
class Model;
class BulletDebugDrawer;
class Light;
class AIMovementGrid;
class TriggerInterface;
class GUIText;

class GUIRenderable;
class GUILayer;
class GUITextBase;
class GUIFPSCounter;
class GUITextDynamic;
class Cursor;


class GameObject;
class Player;
class PhysicalPlayer;
class FreeMovingPlayer;
class FreeCursorPlayer;
class ImGuiHelper;
class AssetManager;
class TriggerObject;
class AnimationCustom;
class AnimationNode;
class AnimationSequenceInterface;
class LimonAPI;

class World {

    struct AnimationStatus {
        PhysicalRenderable* object = nullptr;
        uint32_t animationIndex;
        bool loop;
        long startTime;
        Transformation originalTransformation;
        bool wasKinematic;
    };

    struct ActionForOnload {
        TriggerInterface* action = nullptr;
        std::vector<LimonAPI::ParameterRequest> parameters;
        bool enabled = false;
    };

    enum collisiontypes {
        COLLIDE_NOTHING         = 0, //Collide with nothing
        COLLIDE_EVERYTHING      = 1 << 0, //Pick object etc needs to collide everything
        COLLIDE_MODELS          = 1 << 1, //Collide with All Models
        COLLIDE_PLAYER          = 1 << 2, //Collide with Player
        COLLIDE_AI              = 1 << 3, //Collide with Only AI driven Models
        COLLIDE_TRIGGER_VOLUME  = 1 << 4, //Collide with Triggers

    };

    friend class WorldLoader;
    friend class WorldSaver; //Those classes require direct access to some of the internal data

    enum PlayerModes {DEBUG_MODE, EDITOR_MODE, PHYSICAL_MODE, PAUSED_MODE}; //PAUSED mode is used by quit logic
    AssetManager* assetManager;
    Options* options;
    uint32_t nextWorldID = 1;
    std::map<uint32_t, PhysicalRenderable *> objects;
    std::map<uint32_t, GUIText*> guiElements;
    std::map<uint32_t, TriggerObject*> triggers;
    std::vector<ActionForOnload* > onLoadActions;
    std::vector<AnimationCustom> loadedAnimations;
    std::set<PhysicalRenderable*> onLoadAnimations;//Those renderables animations should be loaded and started on load
    std::unordered_map<PhysicalRenderable*, AnimationStatus> activeAnimations;
    AnimationSequenceInterface* animationInProgress = nullptr;
    std::vector<Light *> lights;
    std::vector<GUILayer *> guiLayers;
    std::unordered_map<uint32_t, Actor*> actors;
    AIMovementGrid *grid = nullptr;
    SkyBox *sky = nullptr;
    GLHelper *glHelper;
    long gameTime = 0;
    glm::vec3 worldAABBMin= glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAABBMax = glm::vec3(std::numeric_limits<float>::min());

    GLSLProgram *shadowMapProgramDirectional, *shadowMapProgramPoint;
    FontManager fontManager;
    PhysicalPlayer* physicalPlayer;
    FreeCursorPlayer* editorPlayer = nullptr;
    FreeMovingPlayer* debugPlayer = nullptr;
    Player* currentPlayer;
    Camera* camera;
    BulletDebugDrawer *debugDrawer;
    GameObject::ImGuiRequest* request;

    GUILayer *apiGUILayer;
    GUIText* renderCounts;
    GUIFPSCounter* fpsCounter;
    Cursor* cursor;
    GUITextDynamic* debugOutputGUI;

    btGhostPairCallback *ghostPairCallback;
    btDiscreteDynamicsWorld *dynamicsWorld;
    std::vector<btRigidBody *> rigidBodies;

    LimonAPI* apiInstance;

    btBroadphaseInterface *broadphase;
    btDefaultCollisionConfiguration *collisionConfiguration;
    btCollisionDispatcher *dispatcher;
    btSequentialImpulseConstraintSolver *solver;
    PlayerModes currentMode = PHYSICAL_MODE;
    PlayerModes beforeMode = PHYSICAL_MODE;
    ImGuiHelper *imgGuiHelper;
    GameObject* pickedObject = nullptr;
    bool availableAssetsLoaded = false;
    bool isQuitRequest = false;//does the player requested a quit?
    bool isQuitVerified = false;//does the player set it is sure?
    bool guiPickMode = false;

    /**
     * This method checks, if IDs assigned without any empty space, and any collision
     * and sets the totalObjectCount accordingly.
     * @return true if everything ok, false if not
     */
    bool verifyIDs(){
        std::set<uint32_t > usedIDs;
        uint32_t maxID;
        /** there are 3 places that has IDs,
         * 1) sky
         * 2) objects
         * 3) AIs
         */
        //put sky first, since it is guaranteed to be single
        usedIDs.insert(this->sky->getWorldObjectID());
        maxID = this->sky->getWorldObjectID();

        for(auto object = objects.begin(); object != objects.end(); object++) {
            auto result = usedIDs.insert(object->first);
            if(result.second == false) {
                std::cerr << "world ID repetition on object detected! with id " << object->first << std::endl;
                return false;
            }
            maxID = std::max(maxID,object->first);
        }

        for(auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
            auto result = usedIDs.insert(trigger->first);
            if(result.second == false) {
                std::cerr << "world ID repetition on trigger detected! with id " << trigger->first << std::endl;
                return false;
            }
            maxID = std::max(maxID,trigger->first);
        }

        for(auto actor = actors.begin(); actor != actors.end(); actor++) {
            auto result = usedIDs.insert(actor->first);
            if(result.second == false) {
                return false;
            }
            maxID = std::max(maxID,actor->first);
        }

        for(uint32_t index = 1; index <= maxID; index++) {
            if(usedIDs.count(index) != 1) {
                //TODO this should be ok, logging just to check. Can be removed in the future
                std::cout << "found empty ID" << index << std::endl;
            }
        }

        nextWorldID = maxID+1;
        return true;
    }

    bool handlePlayerInput(InputHandler &inputHandler);

    bool checkPlayerVisibility(const glm::vec3 &from, const std::string &fromName);

    ActorInformation fillActorInformation(Actor *actor);

    void updateWorldAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);

    void addModelToWorld(Model *xmlModel);

    GameObject * getPointedObject() const;


    void addActor(Actor *actor);

    void createGridFrom(const glm::vec3 &aiGridStartPoint);

    void setSky(SkyBox *skyBox);

    void addLight(Light *light);

    World(AssetManager *assetManager, GLHelper *, Options *options);

    void afterLoadFinished();

    void switchToEditorMode(InputHandler &inputHandler);

    void switchToPhysicalPlayer(InputHandler &inputHandler);

    void switchToDebugMode(InputHandler &inputHandler);

    void ImGuiFrameSetup();

    //API methods

public:
    ~World();

    bool play(Uint32, InputHandler &);

    void render();

    uint32_t getNextObjectID() {
        return nextWorldID++;
    }

    void addAnimationDefinitionToEditor();

    /************************************ Methods LimonAPI exposes *************/
    /**
    * This method fills the parameters required to run the trigger
    * @param runParameters
    * @return true if all requied parameters are set, otherwise false
    */
    bool generateEditorElementsForParameters(std::vector<LimonAPI::ParameterRequest> &runParameters, uint32_t index);

    uint32_t addAnimationToObject(uint32_t modelID, uint32_t animationID, bool looped, bool startOnLoad = false);
    uint32_t addGuiText(const std::string &fontFilePath, uint32_t fontSize,
                        const std::string &name, const std::string &text,
                        const glm::vec3 &color,
                        const glm::vec2 &position, float rotation);

    uint32_t updateGuiText(uint32_t guiTextID, const std::string &newText);

    uint32_t removeObject(uint32_t objectID);
    uint32_t removeTriggerObject(uint32_t triggerobjectID);
    uint32_t removeGuiText(uint32_t guiElementID);

    std::vector<LimonAPI::ParameterRequest> getResultOfTrigger(uint32_t triggerObjectID, uint32_t triggerCodeID);

    bool disconnectObjectFromPhysics(uint32_t objectWorldID);

    bool reconnectObjectToPhysics(uint32_t objectWorldID);

    /************************************ Methods LimonAPI exposes *************/
};

#endif //LIMONENGINE_WORLD_H
