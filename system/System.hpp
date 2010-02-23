#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <celsus/D3DTypes.hpp>
#include <atlbase.h>
#include "SystemInterface.hpp"
#include "ResourceManager.hpp"
#include "HandleManager.hpp"
#include "Input.hpp"
#include "Serializer.hpp"
#include "DirWatcher.hpp"

class MaterialManager;
class ResourceManager;
class FreeFlyCamera;

class System : public SystemInterface 
{
public:
  System();
  ~System();
  virtual bool  init();
  virtual bool  run();
  virtual bool  close();

  virtual void add_event_handler(const EventId::Enum id, const EventHandler& handler);
  virtual void remove_event_handler(const EventId::Enum id, const EventHandler& handler);
  virtual void send_event(const EventId::Enum id, const EventArgs* event_args = NULL);
  virtual void watch_file(const char* filename, const FileChangedCallback &cb);
  virtual void unwatch_file(const char* filename, const FileChangedCallback &cb);

  virtual CTwBar*  get_tweak_bar() { return tweak_bar_; }

  virtual Handle create_vertex_buffer(uint8_t* data, const uint32_t vertex_count, const uint32_t vertex_size);
  virtual Handle create_index_buffer(uint8_t* data, const uint32_t index_count, const uint32_t index_size);
  virtual Handle create_input_layout(D3D10_INPUT_ELEMENT_DESC* descs, const uint32_t num_descs, const D3D10_PASS_DESC& pass_desc);
  virtual Handle load_effect(const std::string& filename);
  virtual void set_input_layout(const Handle handle);
  virtual void add_renderable(Renderable* renderable);
  virtual void set_render_targets(const RenderTargets& render_targets);

  virtual void set_vertex_buffer(const Handle& vb);
  virtual void set_index_buffer(const Handle& ib);
  virtual void set_primitive_topology(const D3D10_PRIMITIVE_TOPOLOGY top);
  virtual void draw_indexed(const uint32_t index_count, const uint32_t start_index_location, const uint32_t base_vertex_location);

  virtual void get_free_fly_camera(D3DXVECTOR3& eye_pos, D3DXMATRIX& view_mtx);

  virtual void set_active_effect(const Handle handle);
  virtual void set_active_effect(LPD3D10EFFECT effect);
  virtual void set_active_technique(const std::string& name);
  virtual bool get_technique_pass_desc(const Handle effect_handle, const std::string& technique_name, D3D10_PASS_DESC& desc);

  virtual void load_texture(const std::string& name);
  virtual void reset();

  virtual void add_process_input_callback(const ProcessInputCallback& cb);

  virtual void set_variable(const std::string& name, const float value);
  virtual void set_variable(const std::string& name, const D3DXMATRIX& value);
  virtual void set_variable(const std::string& name, const D3DXVECTOR2& value);
  virtual void set_variable(const std::string& name, const D3DXVECTOR3& value);
  virtual void set_variable(const std::string& name, const D3DXVECTOR4& value);
  virtual void set_variable(const std::string& name, const D3DXCOLOR& value);
  virtual void set_resource(const std::string& name, const std::string& resource_name);

  virtual void key_down(const uint32_t key);
  virtual void key_up(const uint32_t key);
  virtual void mouse_click(const bool left_button_down, const bool right_button_down);
  virtual void mouse_move(const int x_pos, const int x_last, const int y_pos, const int y_last);

  virtual ID3D10Device* get_device();

  bool    init_directx(int32_t hwnd);
  virtual bool render();

  HandleManager* handle_manager() { return &handle_manager_; }  
  ResourceManager* resource_manager() { return &resource_manager_; }
  const D3D10_VIEWPORT& viewport() { return viewport_; }



private:
  void    process_deferred_events();
  bool    init_dir_watcher();
  bool    close_dir_watcher();
  bool    create_window();
  void    calc_fps(const double duration);
  void    file_changed(const EventArgs* args);
  ID3D10Effect* create_d3d_effect(const char* hlsl, const std::string& filename);

  static LRESULT CALLBACK wnd_proc( HWND, UINT, WPARAM, LPARAM );

  // Event handling
  typedef std::multimap<EventId::Enum, EventHandler> EventHandlers;
  typedef std::pair< EventHandlers::iterator, EventHandlers::iterator> EventHandlerRange;
  typedef std::vector< std::pair<EventId::Enum, const EventArgs*> > DeferredEvents;

  typedef std::multimap<std::string, FileChangedCallback> FileChangedCallbacks;
  typedef std::pair< FileChangedCallbacks::iterator, FileChangedCallbacks::iterator > FileChangedCallbackskRange;

  DeferredEvents _deferred_events;
  EventHandlers _event_handlers;

  FileChangedCallbacks _file_changed_callbacks;

  // End event handling

  typedef std::vector<Renderable*> Renderables;

  HWND        wnd_;
  ATOM        class_atom_;

  IDXGISwapChainPtr swap_chain_;
  ID3D10Texture2DPtr  depth_buffer_;
  ID3D10RenderTargetViewPtr render_target_view_;
  ID3D10DepthStencilViewPtr depth_stencil_view_;
  CComPtr<ID3DX10Font> font_;
  uint32_t last_render_;
  uint32_t frame_count_;
  std::string fps_str_;
  ResourceManager resource_manager_;

  Renderables renderables_;
  HandleManager handle_manager_;
  ID3D10Effect* active_effect_;

  Input input_;

  CRITICAL_SECTION _cs_event;
  DWORD _main_thread_id;
  WatcherThreadParams _thread_params;
  HANDLE _watcher_thread;

  // HACK
  typedef stdext::hash_map<std::string, ID3D10ShaderResourceViewPtr> Textures;
  Textures textures_;

  boost::scoped_ptr<FreeFlyCamera> free_fly_camera_;
  std::vector<ProcessInputCallback> process_input_callbacks;
  bool display_fps_;
  bool  display_tweak_bar_;
  uint32_t elapsed_time_;
  uint32_t last_time_;
  bool paused_;
  bool  first_frame_;
  CTwBar*  tweak_bar_;

  D3D10_VIEWPORT  viewport_;

  //RTTI( System, MEMBER(display_tweak_bar_) MEMBER(display_fps_) );
};

#endif // #ifndef SYSTEM_HPP
