#pragma once

#include <Tempest/Matrix4x4>
#include <Tempest/Texture2d>
#include <array>

#include "graphics/ubochain.h"
#include "resources.h"

class RendererStorage;
class World;

class Sky final {
  public:
    using Vertex=Resources::VertexFsq;

    Sky(const RendererStorage &storage);

    void setWorld(const std::string& world);

    void setMatrix(uint32_t frameId,const Tempest::Matrix4x4& mat);
    void commitUbo(uint32_t frameId);
    void draw(Tempest::CommandBuffer &cmd, uint32_t frameId, const World &world);

    struct Layer final {
      const Tempest::Texture2d* texture=nullptr;
      float                     speed[2]={};
      float                     scale=1.f;
      float                     alpha=1.f;
      };

    struct State final {
      Layer               lay[2];
      std::array<float,3> colorA, colorB, fog;
      bool                day=true;
      };

  private:
    struct UboGlobal {
      Tempest::Matrix4x4 mvp;
      float              dxy[2]={};
      };

    static std::array<float,3>    mkColor(uint8_t r,uint8_t g,uint8_t b);

    const RendererStorage&        storage;
    UboGlobal                     uboCpu;
    UboChain<UboGlobal>           uboGpu;
    Tempest::VertexBuffer<Vertex> vbo;

    State                         day;

    static std::array<float,3>    color;
    static std::array<Vertex,6>   fsq;
  };