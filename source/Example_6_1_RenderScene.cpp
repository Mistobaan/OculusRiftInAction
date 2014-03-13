#include "Common.h"

struct PerEyeArg {
  Eye eye;
  glm::mat4 projectionOffset;
  glm::mat4 modelviewOffset;

  PerEyeArg(Eye eye, const OVR::HMDInfo & ovrHmdInfo)
    : eye(eye) { 
    OVR::Ptr<OVR::ProfileManager> profileManager = *OVR::ProfileManager::Create();
    OVR::Ptr<OVR::Profile> profile = *(profileManager->GetDeviceDefaultProfile(OVR::ProfileType::Profile_RiftDK1));
    float ipd = profile->GetIPD();
    modelviewOffset = glm::translate(glm::mat4(),
      glm::vec3(ipd / 2.0f, 0, 0));
    if (eye == RIGHT) {
      modelviewOffset = glm::inverse(modelviewOffset);
    }
    profileManager.Clear();

    OVR::Util::Render::StereoConfig config;
    config.SetHMDInfo(ovrHmdInfo);
    projectionOffset = glm::translate(glm::mat4(),
      glm::vec3(config.GetProjectionCenterOffset(), 0, 0));
    if (eye == RIGHT) {
      projectionOffset = glm::inverse(projectionOffset);
    }
  }
};

class SimpleScene: public RiftGlfwApp {
  glm::mat4 player;
  glm::mat4 projection;
  gl::TextureCubeMapPtr skyboxTexture;
  float ipd{ 0.06f };
  float eyeHeight{ 1.0f };
  bool applyProjectionOffset{ true };
  bool applyModelviewOffset{ true };
  const std::array<PerEyeArg, 2> eyes;
  
public:
  SimpleScene() : eyes({ { PerEyeArg(LEFT, ovrHmdInfo), PerEyeArg(RIGHT, ovrHmdInfo) } }) {
    OVR::Ptr<OVR::ProfileManager> profileManager = *OVR::ProfileManager::Create();
    OVR::Ptr<OVR::Profile> profile = *(profileManager->GetDeviceDefaultProfile(OVR::ProfileType::Profile_RiftDK1));
    ipd = profile->GetIPD();
    eyeHeight = profile->GetEyeHeight();

    {
      OVR::Util::Render::StereoConfig config;
      config.SetHMDInfo(ovrHmdInfo);
      gl::Stacks::projection().top() =
        glm::perspective(config.GetYFOVRadians(), eyeAspect, 0.01f, 1000.0f);
    }

    glm::vec3 playerPosition(0, eyeHeight, ipd * 4.0f);
    player = glm::inverse(glm::lookAt(playerPosition, glm::vec3(0, eyeHeight, 0), GlUtils::Y_AXIS));
    CameraControl::instance().enableHydra(true);
  }

  virtual ~SimpleScene() {
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);
    skyboxTexture = GlUtils::getCubemapTextures(Resource::IMAGES_SKY_CITY_XNEG_PNG);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    // Handles WASD keys for movement, as well as Razer Hydra, joystick & spacemouse
    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }

    if (action == GLFW_PRESS) switch (key) {
    case GLFW_KEY_P:
      applyProjectionOffset = !applyProjectionOffset;
      return;

    case GLFW_KEY_M:
      applyModelviewOffset = !applyModelviewOffset;
      return;

    case GLFW_KEY_R:
      player = glm::inverse(glm::lookAt(glm::vec3(0, eyeHeight, ipd * 4.0f), glm::vec3(0, eyeHeight, 0), GlUtils::Y_AXIS));
      return;
    }

    RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FOR_EACH_EYE(eye) {
      const PerEyeArg & eyeArgs = eyes[eye];
      viewport(eye);
      renderScene(eyeArgs);
    }
  }

  void renderScene(const PerEyeArg & eyeArg) {
    gl::MatrixStack & pr = gl::Stacks::projection();
    gl::MatrixStack & mv = gl::Stacks::modelview();
    pr.push();
    mv.push();
    if (applyProjectionOffset) {
      pr.preMultiply(eyeArg.projectionOffset);
    }
    if (applyModelviewOffset) {
      mv.preMultiply(eyeArg.modelviewOffset);
    }

    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(player);

    mv.push().translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
    GlUtils::drawColorCube(true);
    mv.pop();

    mv.pop();
    pr.pop();
  }
};

RUN_OVR_APP(SimpleScene);