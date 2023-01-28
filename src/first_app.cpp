#include "first_app.hpp"

#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "lve_camera.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <iostream>

// jam
#include "jam_CASystem.hpp"
namespace lve {

FirstApp::FirstApp() {
  globalPool =
      LveDescriptorPool::Builder(lveDevice)
          .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
          .build();
  loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {

  
  
  // ***_ JAM start _*** //
  int caRows = 40;
  int caCols = 40;
  jam::CASystem cellularAutomata{caRows, caCols};
  std::shared_ptr<LveModel> myCubeModel = LveModel::createModelFromFile(lveDevice, "models/cube.obj");
  std::vector<std::vector<lve::LveGameObject*>> gameObjectsGrid;
  gameObjectsGrid.resize(caRows, std::vector<lve::LveGameObject*>(caCols));
  for (int i = 0; i < caRows; i++) {
      for (int j = 0; j < caCols; j++) {
        auto newGameObj = LveGameObject::createGameObject();
        newGameObj.model = myCubeModel;
        float scaleFactor = 0.21;
        newGameObj.transform.translation = { float(i) * scaleFactor, float(-j) * scaleFactor, 0.f};
        newGameObj.transform.scale = {0.1f, 0.1f, 0.1f};
        gameObjects.emplace(newGameObj.getId(), std::move(newGameObj));
        gameObjectsGrid[i][j] = &gameObjects.at(newGameObj.getId());
      }
  }
  // ***_ JAM end _*** //
  
  
  
  std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] = std::make_unique<LveBuffer>(
        lveDevice,
        sizeof(GlobalUbo),
        1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }

  auto globalSetLayout =
      LveDescriptorSetLayout::Builder(lveDevice)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
          .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < globalDescriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    LveDescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  SimpleRenderSystem simpleRenderSystem{
      lveDevice,
      lveRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout()};
  PointLightSystem pointLightSystem{
      lveDevice,
      lveRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout()};
  LveCamera camera{};

  auto viewerObject = LveGameObject::createGameObject();
  viewerObject.transform.translation.z = -2.5f;
  viewerObject.transform.translation.y = -1.3f;
  KeyboardMovementController cameraController{};
  auto currentTime = std::chrono::high_resolution_clock::now();
  float caStepTimer = 0.f;
  float caResetTimer = 0.f;
  while (!lveWindow.shouldClose()) {
    glfwPollEvents();
    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
    currentTime = newTime;  
      
  
  
      // ***_ JAM start _*** //
      caStepTimer += frameTime;
      if ( caStepTimer > 1.0f) { // step every seconds.
        cellularAutomata.step();
        caStepTimer = 0.0;
      } 
      caResetTimer += frameTime;
      if ( caResetTimer > 5.0f) { // step every seconds.
        cellularAutomata.initialiseValues();
        caResetTimer = 0.0;
      } 



      // set cube positions :
      for (int i = 0; i < caRows; i++) {
        for (int j = 0; j < caCols; j++) {
          gameObjectsGrid[i][j]->transform.translation.z = float(cellularAutomata.grid[i][j]);
        }
      }
      // ***_ JAM end _*** //



    cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
    camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

    float aspect = lveRenderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

    if (auto commandBuffer = lveRenderer.beginFrame()) {
      int frameIndex = lveRenderer.getFrameIndex();
      FrameInfo frameInfo{
          frameIndex,
          frameTime,
          commandBuffer,
          camera,
          globalDescriptorSets[frameIndex],
          gameObjects};

      // update
      GlobalUbo ubo{};
      ubo.projection = camera.getProjection();
      ubo.view = camera.getView();
      ubo.inverseView = camera.getInverseView();
      pointLightSystem.update(frameInfo, ubo);
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      // render
      lveRenderer.beginSwapChainRenderPass(commandBuffer);

      // order here matters
      simpleRenderSystem.renderGameObjects(frameInfo);
      pointLightSystem.render(frameInfo);

      lveRenderer.endSwapChainRenderPass(commandBuffer);
      lveRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
  
  std::shared_ptr<LveModel> lveModel =
      LveModel::createModelFromFile(lveDevice, "models/flat_vase.obj");
  auto flatVase = LveGameObject::createGameObject();
  flatVase.model = lveModel;
  flatVase.transform.translation = {1.f, .0f, 0.f};
  flatVase.transform.scale = {0.1f, 0.1f, 0.1f};
  gameObjects.emplace(flatVase.getId(), std::move(flatVase));

  lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
  auto smoothVase = LveGameObject::createGameObject();
  smoothVase.model = lveModel;
  smoothVase.transform.translation = {.0f, 0.0f, 1.f};
  smoothVase.transform.scale = {0.1f, 0.1f, 0.1f};
  gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

  lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
  auto floor = LveGameObject::createGameObject();
  floor.model = lveModel;
  floor.transform.translation = {0.f, .0f, 0.f};
  floor.transform.scale = {300.f, 1.f, 300.f};
  gameObjects.emplace(floor.getId(), std::move(floor));

  lveModel = LveModel::createModelFromFile(lveDevice, "models/cube.obj");
  auto cube = LveGameObject::createGameObject();
  cube.model = lveModel;
  cube.color = glm::vec3(1.f, 0.1f,0.1f);
  cube.transform.translation = {0.0f, 0.0f, 0.0f};
  cube.transform.scale = {0.1f, 0.1f, 0.1f};
  gameObjects.emplace(cube.getId(), std::move(cube));

  std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f},
      {.1f, .1f, 1.f},
      {.1f, 1.f, .1f},
      {1.f, 1.f, .1f},
      {.1f, 1.f, 1.f},
      {1.f, 1.f, 1.f}  //
  };

  for (int i = 0; i < lightColors.size(); i++) {
    auto pointLight = LveGameObject::makePointLight(0.2f);
    pointLight.color = lightColors[i];
    auto rotateLight = glm::rotate(
        glm::mat4(1.f),
        (i * glm::two_pi<float>()) / lightColors.size(),
        {0.f, -1.f, 0.f});
    pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
    gameObjects.emplace(pointLight.getId(), std::move(pointLight));
  }
}

}  // namespace lve
