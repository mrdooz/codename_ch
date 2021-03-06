#ifndef DEBUG_RENDERER_HPP
#define DEBUG_RENDERER_HPP

#include <celsus/D3DTypes.hpp>
#include "VectorFont.hpp"

class EffectWrapper;

struct PosCol
{
  PosCol() {}
  PosCol(const D3DXVECTOR3& pos, const D3DXCOLOR& col) : pos(pos), col(col) {}
  PosCol(const D3DXVECTOR3& pos) : pos(pos) {}
  D3DXVECTOR3 pos;
  D3DXCOLOR col;
};

class DebugRenderer
{
public:
  enum {
    Pos = 1 << 0,
    Normal = 1 << 1,
    Color = 1 << 2,
  };

  DebugRenderer(ID3D10DevicePtr device);
  ~DebugRenderer();

  bool init();
  void close();
  void start_frame();
  void end_frame();
  // vertex_format is a bitmask made from Pos, Normal, Color
  void add_verts(const uint32_t vertex_format, const D3D10_PRIMITIVE_TOPOLOGY topology, const float* verts, const uint32_t vertex_count, 
    const D3DXMATRIX& world, const D3DXMATRIX& view_proj);
  void render();

  void add_wireframe_sphere(const D3DXVECTOR3& center, const float radius, const D3DXCOLOR& color, const D3DXMATRIX& world, const D3DXMATRIX& view_proj);
  void add_wireframe_sphere(const D3DXMATRIX& world, const D3DXCOLOR& color, const D3DXMATRIX& view_proj);

  void  add_line(const D3DXVECTOR3& a, const D3DXVECTOR3& b, const D3DXCOLOR& color, const D3DXMATRIX& view_proj);
  void  add_text(const D3DXVECTOR3& pos, const D3DXMATRIX& view, const D3DXMATRIX& proj, const bool billboard, const char* format, ...);

  void  add_debug_string(const char* format, ...);

private:
  struct DrawCall
  {
    DrawCall(const uint32_t vertex_count, const uint32_t start_vertex, const D3DXMATRIX& view_proj) 
      : vertex_count(vertex_count), start_vertex_location(start_vertex), view_proj(view_proj) {}
    uint32_t  vertex_count;
    uint32_t  start_vertex_location;
    D3DXMATRIX  view_proj;
  };

  void draw_line(const float* proj, float x1,float y1,float z1, float x2,float y2, float z2);

  typedef std::vector<DrawCall> DrawCalls;
  typedef std::map< D3D10_PRIMITIVE_TOPOLOGY, DrawCalls > DrawCallsByTopology;

  struct VertexFormatData
  {
    VertexFormatData()
      : data_(NULL)
      , vertex_size_(0)
      , num_verts(0)
      , buffer_size_(0)
      , data_ofs_(0)
      , input_layout_(NULL)
      , vertex_count_(0)
    {
    }
    uint32_t vertex_size_;
    uint32_t num_verts;
    uint32_t buffer_size_;
    uint32_t data_ofs_;
    uint8_t* data_;
    uint32_t vertex_count_;
    std::string technique_name_;

    DrawCallsByTopology draw_calls_by_topology_;

    ID3D10BufferPtr vertex_buffer_;
    ID3D10InputLayoutPtr input_layout_;
  };

  bool init_vertex_buffers();
  void init_unit_sphere();
  void  color_sphere(const D3DXCOLOR& col);

  ID3D10Buffer* create_dynamic_vertex_buffer(const uint32_t vertex_count, const uint32_t vertex_size);
  typedef std::map< uint32_t, VertexFormatData > VertexFormats;
  VertexFormats vertex_formats_;
  ID3D10DevicePtr device_;
  EffectWrapper* effect_;

  ID3D10BlendStatePtr blend_state_;
  ID3D10DepthStencilStatePtr  depth_stencil_state_;

  std::vector<PosCol> sphere_verts_;
  ID3DX10FontPtr  font_;
  VectorFont* vector_font_;

  std::vector< std::string >  debug_text_;

};


#endif
