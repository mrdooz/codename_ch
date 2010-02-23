#ifndef SYSTEM_INTERFACE_HPP
#define SYSTEM_INTERFACE_HPP

#include <vector>
#include <string>
#include <D3DX10.h>

#include "Event.hpp"

#ifdef STANDALONE
#define SYS_PURE(x) = 0
#else
#define SYS_PURE(x) x
#pragma warning(push)
#pragma warning(disable: 4100 4121 4244 4512)
#include <boost/python.hpp>
#pragma warning(pop)
typedef boost::python::object PyObj;
#endif
#include <boost/function.hpp>

#include "Handle.hpp"
#include "Renderable.hpp"

struct Input;
class ResourceManager;

typedef std::vector< std::pair<int32_t, std::string> > RenderTargets;
typedef std::string MaterialName;
typedef std::string MeshName;
typedef boost::function<void (const MeshName&, const MaterialName&)> MaterialChangedCallback;
typedef boost::function<void (const MeshName&)> SelectedObjectChangedCallback;
typedef boost::function<void (const Input&)> ProcessInputCallback;

struct CTwBar;

// These functions should really be pure virtual, but it'll have to wait until I get my head around the
// call wrapper stuff in boost python.
struct SystemInterface 
{
  virtual ~SystemInterface() {};
  virtual bool init() SYS_PURE( { return true; } );
  virtual bool run() SYS_PURE( { return true; } );
  virtual bool close() SYS_PURE( { return true; } );

  virtual void add_event_handler(const EventId::Enum /*id*/, const EventHandler& /*handler*/) = 0;
  virtual void remove_event_handler(const EventId::Enum /*id*/, const EventHandler& /*handler*/) = 0;
  virtual void send_event(const EventId::Enum /*id*/, const EventArgs* /*event_args*/ = NULL) = 0;
  virtual void watch_file(const char* /*filename*/, const FileChangedCallback &/*cb*/) = 0;
  virtual void unwatch_file(const char* /*filename*/, const FileChangedCallback &/*cb*/) = 0;

  virtual CTwBar*  get_tweak_bar() SYS_PURE( { return NULL; } );

  virtual Handle create_vertex_buffer(uint8_t* /*data*/, const uint32_t /*vertex_count*/, const uint32_t /*vertex_size*/) SYS_PURE({ return Handle(); });
  virtual Handle create_index_buffer(uint8_t* /*data*/, const uint32_t /*index_count*/, const uint32_t /*index_size*/) SYS_PURE({ return Handle(); });
  virtual Handle create_input_layout(D3D10_INPUT_ELEMENT_DESC* /*descs*/, const uint32_t /*num_descs*/, const D3D10_PASS_DESC& /*pass_desc*/) SYS_PURE({ return Handle();});
  virtual Handle load_effect(const std::string& /*filename*/) SYS_PURE({ return Handle(); });

  virtual void set_input_layout(const Handle /*handle*/) SYS_PURE({});
  virtual void add_renderable(Renderable* renderable) SYS_PURE({});
  virtual void set_render_targets(const RenderTargets& render_targets) SYS_PURE({});
  virtual void set_primitive_topology(const D3D10_PRIMITIVE_TOPOLOGY top) SYS_PURE({});

  virtual void get_free_fly_camera(D3DXVECTOR3& eye_pos, D3DXMATRIX& view_mtx) SYS_PURE({});

  virtual void set_vertex_buffer(const Handle& vb) SYS_PURE({});
  virtual void set_index_buffer(const Handle& ib) SYS_PURE({});
  virtual void draw_indexed(const uint32_t index_count, const uint32_t start_index_location, const uint32_t base_vertex_location) SYS_PURE({});

  virtual void set_active_effect(const Handle handle) SYS_PURE({});
  virtual void set_active_effect(LPD3D10EFFECT effect) SYS_PURE({});

  virtual void set_active_technique(const std::string& name) SYS_PURE({});
  virtual bool get_technique_pass_desc(const Handle effect_handle, const std::string& technique_name, D3D10_PASS_DESC& desc) SYS_PURE({ return false; });
  virtual void load_texture(const std::string& name) SYS_PURE({});
  virtual void reset() SYS_PURE({});

  virtual void add_process_input_callback(const ProcessInputCallback& cb) SYS_PURE({});



  // remove this later, but for now i want it for the state object builders.
  virtual ID3D10Device* get_device() SYS_PURE({ return NULL; });

  virtual void key_down(const uint32_t key) SYS_PURE({});
  virtual void key_up(const uint32_t key) SYS_PURE({});
  virtual void mouse_click(const bool left_button_down, const bool right_button_down) SYS_PURE({});
  virtual void mouse_move(const int x_pos, const int x_last, const int y_pos, const int y_last) SYS_PURE({});

  virtual void set_variable(const std::string& name, const float value) SYS_PURE({});
  virtual void set_variable(const std::string& name, const D3DXMATRIX& value) SYS_PURE({});
  virtual void set_variable(const std::string& name, const D3DXVECTOR2& value) SYS_PURE({});
  virtual void set_variable(const std::string& name, const D3DXVECTOR3& value) SYS_PURE({});
  virtual void set_variable(const std::string& name, const D3DXVECTOR4& value) SYS_PURE({});
  virtual void set_variable(const std::string& name, const D3DXCOLOR& value) SYS_PURE({});
  virtual void set_resource(const std::string& name, const std::string& resource_name) SYS_PURE({});

  virtual ResourceManager* resource_manager()  SYS_PURE({});
  virtual const D3D10_VIEWPORT& viewport() SYS_PURE({});

  virtual bool render() = 0;

};

extern SystemInterface *g_system;

#ifdef STANDALONE
extern "C"
{
  __declspec( dllexport ) SystemInterface* create_system();
}
#endif

#endif // #ifndef SYSTEM_INTERFACE_HPP
