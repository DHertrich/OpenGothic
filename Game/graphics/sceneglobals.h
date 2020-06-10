#pragma once

#include <Tempest/Matrix4x4>
#include <Tempest/Vec>
#include <list>

#include "light.h"
#include "ubochain.h"

class RendererStorage;
class ObjectsBucket;

class SceneGlobals final {
  public:
    SceneGlobals(const RendererStorage& storage);
    ~SceneGlobals();

    void setModelView(const Tempest::Matrix4x4& m, const Tempest::Matrix4x4 *sh, size_t shCount);
    void commitUbo(uint8_t fId);

    const RendererStorage&          storage;
    uint64_t                        tickCount = 0;
    const Tempest::Texture2d*       shadowMap = &Resources::fallbackBlack();

  //private:
    struct UboGlobal final {
      Tempest::Vec3                 lightDir={0,0,1};
      float                         padding=0;
      Tempest::Matrix4x4            modelView;
      Tempest::Matrix4x4            shadowView;
      Tempest::Vec4                 lightAmb={0,0,0,0};
      Tempest::Vec4                 lightCl ={1,1,1,0};
      };

    Tempest::UniformBuffer<UboGlobal> uboGlobalPf[Resources::MaxFramesInFlight][Resources::ShadowLayers];
    UboGlobal                         uboGlobal;

    Tempest::Matrix4x4              shadowView1;

    Light                           sun;
    Tempest::Vec3                   ambient;
  };

