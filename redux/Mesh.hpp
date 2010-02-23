#ifndef MESH_HPP
#define MESH_HPP

#include "ReduxTypes.hpp"
#include "../system/Handle.hpp"
#include "../system/SystemInterface.hpp"

extern ID3D10Device* g_d3d_device;

struct SystemInterface;

class Mesh 
{
public:
  ~Mesh();

  void create_input_layout(SystemInterface& system, const D3D10_PASS_DESC& pass_desc);
  const std::string name() { return name_; }
  const std::string& transform_name() const { return transform_name_; }
  AnimationNodeSPtr animation_node() const { return animation_node_; }
  void render(SystemInterface& system);
  void  render();

  D3DXVECTOR3 bounding_sphere_center() const;
  float bounding_sphere_radius() const;

  void set_input_layout(ID3D10InputLayout* layout);
private:
  friend class FbxProxy;
  friend class ReduxLoader;
  friend class M2Loader;
  friend struct Scene;

  /**/            Mesh(const std::string& name);
  std::string     name_;
  std::string transform_name_;
  AnimationNodeSPtr animation_node_;

  UINT            index_count_;
  UINT            vertex_buffer_stride_;
  DXGI_FORMAT     index_buffer_format_;

  D3DXVECTOR3 bounding_sphere_center_;
  float bounding_sphere_radius_;

  std::vector<D3D10_INPUT_ELEMENT_DESC>  input_element_descs_;
  Handle  input_layout_;
  ID3D10InputLayout*  input_layout2_;
  ID3D10Buffer* vertex_buffer_;
  ID3D10Buffer* index_buffer_;

  struct DrawCall
  {
    DrawCall(const uint32_t index_count, const uint32_t start_idx, const uint32_t base_vtx) 
      : index_count(index_count), start_idx(start_idx), base_vtx(base_vtx) {}
    uint32_t  index_count;
    uint32_t  start_idx;
    uint32_t  base_vtx;
  };

  std::vector<DrawCall> draw_calls;

};

inline D3DXVECTOR3 Mesh::bounding_sphere_center() const
{
  return bounding_sphere_center_;
}

inline float Mesh::bounding_sphere_radius() const
{
  return bounding_sphere_radius_;
}

inline void Mesh::render(SystemInterface& system) 
{
  system.set_input_layout(input_layout_);

  const UINT offset = 0;
  g_d3d_device->IASetIndexBuffer(index_buffer_, index_buffer_format_, 0);
  g_d3d_device->IASetVertexBuffers(0, 1, &vertex_buffer_, &vertex_buffer_stride_, &offset);
  g_d3d_device->DrawIndexed(index_count_, 0, 0);
}

inline void Mesh::render()
{
  assert(input_layout2_ != NULL);
  const UINT offset = 0;
  g_d3d_device->IASetInputLayout(input_layout2_);
  g_d3d_device->IASetIndexBuffer(index_buffer_, index_buffer_format_, 0);
  g_d3d_device->IASetVertexBuffers(0, 1, &vertex_buffer_, &vertex_buffer_stride_, &offset);

  if (draw_calls.empty()) {
    g_d3d_device->DrawIndexed(index_count_, 0, 0);
  } else {
    for (int32_t i = 0, e = draw_calls.size(); i < e; ++i) {
      g_d3d_device->DrawIndexed(draw_calls[i].index_count, draw_calls[i].start_idx, draw_calls[i].base_vtx);
    }
  }

}

#endif
