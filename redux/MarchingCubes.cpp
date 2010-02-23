#include "stdafx.h"
#include "MarchingCubes.hpp"
#include "Mesh.hpp"
#include "DebugRenderer.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "MarchingCubesUtils.hpp"

using namespace std;
using namespace boost::assign;

extern ID3D10Device* g_d3d_device;

namespace 
{
  const float width = 800;
  const float height = 600;
  const float far_plane = 1000;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;

  //VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> >* vb = NULL;
  VertexBuffer< mpl::vector<D3DXVECTOR3> >* vb = NULL;
  IndexBuffer<uint32_t>* ib = NULL;
}

namespace mpl = boost::mpl;
namespace intrusive = boost::intrusive;

struct Attractor
{
  float value(const D3DXVECTOR3& p) const
  {
    const float d = dist(p.x, p.y, p.z, pos.x, pos.y, pos.z);
    return 5.0f / (d*d);
    return clamp(d, 0.0f, 10.0f);
  }

  D3DXVECTOR3 pos;
};

struct Grid
{

  Grid(const int32_t size)
    : _size(size)
    , _ofs(0,0,0)
    , _scale(1,1,1)
  {
    const int s = _size * _size * _size;
    _grid = new float[s];
    memset(_grid, 0, s);

    Attractor a;
    a.pos.x = 0;
    a.pos.y = 0;
    a.pos.z = 0;

    _attractors.push_back(a);
    _attractors.push_back(a);
    _attractors.push_back(a);
  }

  ~Grid()
  {
    SAFE_ADELETE(_grid);
  }

  float* ptr(const int32_t x, const int32_t y, const int32_t z) const
  {
    const int32_t size2 = _size * _size;
    return _grid + x + y * _size + z * size2;
  }

  float iso_value(const D3DXVECTOR3& p) const
  {
    float res = 0;
    for (int32_t i = 0, e = _attractors.size(); i < e; ++i) {
      res += _attractors[i].value(p);
    }
    return res;
  }

  void update_grid(const int time)
  {
    for (int32_t i = 0, e = _attractors.size(); i < e; ++i) {
      _attractors[i].pos[i] = 10 * sinf((float)time/1000);
    }

    for (int32_t z = 0; z < _size; ++z) {
      for (int32_t y = 0; y < _size; ++y) {
        for (int32_t x = 0; x < _size; ++x) {
          *ptr(x, y, z) = iso_value(iterator_to_pos(x, y, z));
        }
      }
    }
  }

  D3DXVECTOR3 iterator_to_pos(const int32_t x, const int32_t y, const int32_t z) const
  {
    return D3DXVECTOR3(
      _ofs.x + (x - _size / 2) * _scale.x,
      _ofs.y + (y - _size / 2) * _scale.y,
      _ofs.z + (z - _size / 2) * _scale.z
      );
  }

  int _size;
  float* _grid;
  D3DXVECTOR3 _ofs;
  D3DXVECTOR3 _scale;
  std::vector<Attractor> _attractors;

};

int flatten(const int size, const int x, const int y, const int z)
{
  const int size2 = size * size;
  return x + y * size + z * size2;
}

void expand3(const int value, const int size, int* x, int* y, int* z)
{
  const int size2 = size * size;
  *x = value % size;
  *y = (value / size) % size;
  *z = (value / size2) % size;
}

struct GridIterator
{
  GridIterator(Grid* grid)
    : _grid(grid)
    , _iter(0)
    , _num_elems((grid->_size - 1) * (grid->_size - 1) * (grid->_size - 1))
  {

  }

  bool next(GridCell* out)
  {
    if (_iter >= _num_elems) {
      return false;
    }

    int32_t x, y, z;
    expand3(_iter++, _grid->_size-1, &x, &y, &z);

    static int ofs[8][3] = { 
      {+0, +0, +1},
      {+1, +0, +1},
      {+1, +0, +0},
      {+0, +0, +0},
      {+0, +1, +1},
      {+1, +1, +1},
      {+1, +1, +0},
      {+0, +1, +0} };

    for (int32_t i = 0; i < 8; ++i) {
      const int32_t tx = x + ofs[i][0];
      const int32_t ty = y + ofs[i][1];
      const int32_t tz = z + ofs[i][2];
      out->p[i] = _grid->iterator_to_pos(tx, ty, tz);
      out->val[i] = *_grid->ptr(tx, ty, tz);
      }

    return true;
  }

  Grid* _grid;
  int32_t _iter;
  int32_t _num_elems;

};



MarchingCubes::MarchingCubes(const SystemSPtr& system, const EffectManagerSPtr& effect_manager)
  : system_(system)
  , effect_manager_(effect_manager)
  , animation_manager_(new AnimationManager())
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , dynamic_mgr_(new DebugRenderer(g_d3d_device))
  , splits_(20)
  , effect_(g_d3d_device)
  , _grid(new Grid(25))
{

  ib = new IndexBuffer<uint32_t>(g_d3d_device);
  vb = new VertexBuffer< mpl::vector<D3DXVECTOR3> >(g_d3d_device);

  system_->add_renderable(this);
  Serializer::instance().add_instance(this);

  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

}

MarchingCubes::~MarchingCubes()
{
  SAFE_DELETE(vb);
  SAFE_DELETE(ib);

  clear_vector(loaded_effects_);
  clear_vector(loaded_materials_);
  clear_vector(loaded_material_connections_);
  effect_list_.clear();
  effects_.clear();

  SAFE_DELETE(animation_manager_);
  container_delete(effect_connections_);
  system_.reset();
  effect_manager_.reset();
}

bool MarchingCubes::close()
{
  return true;
}

#define RETURN_ON_FALSE(x) if (!(x)) { return false; } else {}

bool MarchingCubes::init() 
{
  RETURN_ON_FALSE(effect_.load("effects/MarchingCubes.fx"));
  RETURN_ON_FALSE(dynamic_mgr_->init());

  D3D10_PASS_DESC desc;
  RETURN_ON_FALSE(effect_.get_pass_desc(desc, "render"));
  RETURN_ON_FALSE(vb->init(desc));
  RETURN_ON_FALSE(ib->init());
  return true;
}
void MarchingCubes::render(const int32_t time_in_ms, const int32_t delta)
{
  D3DXVECTOR3 eye_pos;
  D3DXMATRIX mtx_proj;
  D3DXMATRIX mtx_view;
  system_->get_free_fly_camera(eye_pos, mtx_view);
  D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, 0.1f, far_plane);

  effect_.set_technique("render");
  effect_.set_variable("projection", mtx_proj);
  effect_.set_variable("view", mtx_view);
  effect_.set_variable("eye_pos", eye_pos);
  effect_.set_variable("world", kMtxId);
  effect_.set_variable("world_view_proj", kMtxId * mtx_view * mtx_proj);

  _grid->update_grid(time_in_ms);

  GridCell cell;
  Triangle tris[5];
  GridIterator it(_grid.get());

  vb->start_frame();
  while (it.next(&cell)) {
    if (int32_t tri_count = Polygonise(cell, 1, tris)) {
      for (int32_t i = 0; i < tri_count; ++i) {
        vb->add(tris[i].p[0]);
        vb->add(tris[i].p[1]);
        vb->add(tris[i].p[2]);
      }
    }
  }
  vb->end_frame();

  if (vb->vertex_count() > 0) {
    ib->start_frame();
    for (int32_t i = 0, e = vb->vertex_count(); i < e; ++i) {
      ib->add(i);
    }
    ib->end_frame();

    vb->set_input_layout();
    vb->set_vertex_buffer();
    ib->set_index_buffer();
    g_d3d_device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_d3d_device->DrawIndexed(ib->index_count(), 0, 0);
  }
}

void MarchingCubes::process_input_callback(const Input& input)
{
  // TODO: For this to really work, we need to be able to get the key-up event, so we can use that to toggle
  // switch between different cameras.

  if (scene_.cameras_.empty()) {
    free_fly_camera_enabled_  = true;
  } else {
    if (input.key_status['0']) {
      free_fly_camera_enabled_ = true;
    }

    const uint32_t num_cameras = scene_.cameras_.size();
    for (uint32_t i = 0; i < num_cameras; ++i) {
      if (input.key_status['1' + i]) {
        free_fly_camera_enabled_ = false;
        current_camera_ = i;
        break;
      }
    }
  }

}
