#include "stdafx.h"
#include "System.hpp"
#include "EffectManager.hpp"
#include "FreeFlyCamera.hpp"
#include <celsus/timer.hpp>
#include <AntTweakBar.h>

using namespace std;

ID3D10Device* g_d3d_device = NULL;
SystemInterface *g_system = NULL;

#define LOG_AND_RETURN_HR(x) { HRESULT hr = (x); if (FAILED(hr)) { LOG_WARNING_LN(#x); return false; } }

namespace 
{
  System* g_derived_system = NULL;

  const char* kClassName = "redux";

  NameToTechnique collect_techniques(ID3D10Effect* d3d_effect) 
  {
    NameToTechnique techniques;

    D3D10_EFFECT_DESC effect_desc;
    d3d_effect->GetDesc(&effect_desc);
    for (uint32_t i = 0; i < effect_desc.Techniques; ++i ) {
      ID3D10EffectTechnique* technique = d3d_effect->GetTechniqueByIndex(i);
      D3D10_TECHNIQUE_DESC technique_desc;
      technique->GetDesc(&technique_desc);
      techniques.insert(make_pair(technique_desc.Name, new Technique(technique_desc.Name, technique_desc, technique)));
    }
    return techniques;
  }

  uint32_t WINDOW_WIDTH = 1024;
  uint32_t WINDOW_HEIGHT = 768;
}

extern HINSTANCE g_instance;

System::System()
  : wnd_(0)
  , class_atom_(0)
  , last_render_(0)
  , frame_count_(0)
  , active_effect_(NULL)
  , elapsed_time_(0)
  , first_frame_(true)
  , tweak_bar_(NULL)
  , display_tweak_bar_(true)
  , display_fps_(true)
  , paused_(false)
  , _watcher_thread(INVALID_HANDLE_VALUE)
  , _main_thread_id(GetCurrentThreadId())
{

  g_system = g_derived_system = this;
  InitializeCriticalSection(&_cs_event);

  add_event_handler(EventId::FileChanged, fastdelegate::bind(&System::file_changed, this));

  //Serializer::instance().add_instance(this);
}

System::~System() 
{
  DeleteCriticalSection(&_cs_event);
  //DeleteContainer(bitmaps_);

  if( wnd_ != 0 ) {
    //DestroyWindow(hWnd_);
  }

  if( class_atom_ != 0 ) {
    UnregisterClassA(kClassName, g_instance);
  }
}


void TW_CALL load_callback(void* /*client_data*/)
{
  Serializer::instance().load("codename_ch.dat");
}

void TW_CALL save_callback(void* /*client_data*/)
{
  Serializer::instance().save("codename_ch.dat");
}

bool System::init_directx(int32_t hwnd) 
{
  RECT window_rect;
  GetWindowRect(reinterpret_cast<HWND>(hwnd), &window_rect);
  WINDOW_WIDTH = window_rect.right - window_rect.left;
  WINDOW_HEIGHT = window_rect.bottom - window_rect.top;

  // Create device and swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory( &sd, sizeof(sd) );
  sd.BufferCount = 1;
  sd.BufferDesc.Width = WINDOW_WIDTH;
  sd.BufferDesc.Height = WINDOW_HEIGHT;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
  sd.OutputWindow = reinterpret_cast<HWND>(hwnd); 
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;

  IDXGISwapChain* swap_chain = NULL;

  // Look for 'NVIDIA PerfHUD' adapter
  // If it is present, override default settings
  IDXGIFactory *pDXGIFactory;
  HRESULT hRes;
  hRes = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);
  // Search for a PerfHUD adapter.
  UINT nAdapter = 0;
  IDXGIAdapter* adapter = NULL;
  IDXGIAdapter* selectedAdapter = NULL;

#ifdef _DEBUG
  UINT flags = D3D10_CREATE_DEVICE_DEBUG;
#else
  UINT flags = 0;
#endif

  D3D10_DRIVER_TYPE driverType = D3D10_DRIVER_TYPE_HARDWARE;

  while (pDXGIFactory->EnumAdapters(nAdapter, &adapter) != DXGI_ERROR_NOT_FOUND) {
    if (adapter) {
      DXGI_ADAPTER_DESC adaptDesc;
      if (SUCCEEDED(adapter->GetDesc(&adaptDesc))) {
        const bool isPerfHUD = wcscmp(adaptDesc.Description, L"NVIDIA PerfHUD") == 0;
        // Select the first adapter in normal circumstances or the PerfHUD one if it exists.
        if(nAdapter == 0 || isPerfHUD) {
          selectedAdapter = adapter;
        }
        if(isPerfHUD) {
          driverType = D3D10_DRIVER_TYPE_REFERENCE;
        }
      }
    }
    ++nAdapter;
  }

  LOG_AND_RETURN_HR( D3D10CreateDeviceAndSwapChain( 
    selectedAdapter, driverType, NULL, 
    flags,
    D3D10_SDK_VERSION, &sd, &swap_chain, &g_d3d_device));

  swap_chain_.Attach(swap_chain);

  TwInit(TW_DIRECT3D10, g_d3d_device);
  TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  tweak_bar_ = TwNewBar("codename crazy head!");
  Serializer::instance().bar_ = tweak_bar_;
  TwAddButton(tweak_bar_, "Load", load_callback, NULL, "");
  TwAddButton(tweak_bar_, "Save", save_callback, NULL, "");
  TwAddSeparator(tweak_bar_, "", "");
  free_fly_camera_.reset(new FreeFlyCamera());

  // Create a render target view
  ID3D10Texture2DPtr back_buffer;
  LOG_AND_RETURN_HR(swap_chain_->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&back_buffer ));

  // Create a resource view for the render target
  LOG_AND_RETURN_HR(g_d3d_device->CreateRenderTargetView(back_buffer, NULL, &render_target_view_));

  // Create depth stencil texture
  ID3D10Texture2D* depth_buffer_raw = NULL;
  ID3D10DepthStencilView* depth_stencil_view_raw = NULL;

  LOG_AND_RETURN_HR(
    create_depth_stencil_and_view(depth_buffer_raw, depth_stencil_view_raw, g_d3d_device, WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_D32_FLOAT));
  depth_buffer_.Attach(depth_buffer_raw);
  depth_stencil_view_.Attach(depth_stencil_view_raw);

  // Add the default render target and depth stencil views to the resource manager
  resource_manager_.add_render_target_view(render_target_view_, "default");
  resource_manager_.add_depth_stencil_view(depth_stencil_view_, "default");
  resource_manager_.set_current_depth_stencil("default");

  LOG_AND_RETURN_HR( D3DX10CreateFont( g_d3d_device, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
    OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
    _T("Arial"), &font_ ) );

  // Set the viewport
  g_d3d_device->RSSetViewports( 1, init_d3d10_viewport(&viewport_, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1) );
  return true;
}

bool System::render() 
{
  process_deferred_events();

  Timer timer;
  timer.start();
  for (size_t i = 0; i < process_input_callbacks.size(); ++i) {
    process_input_callbacks[i](input_);
  }

  const uint32_t cur_time = timeGetTime();

  if (first_frame_) {
    last_time_ = cur_time;
    first_frame_ = false;
  }

  const uint32_t delta = cur_time - last_time_;
  if (!paused_) {
    elapsed_time_ += delta;
  }

  if (free_fly_camera_) {
    free_fly_camera_->update_from_input(delta / 1000.0f, input_);
    free_fly_camera_->tick(delta / 1000.0f);

    // Unproject the screen space mouse pos, and calc the ray into the screen
    D3DXVECTOR3 near_pos, far_pos;
    const D3DXMATRIX& proj = free_fly_camera_->proj_matrix();
    const D3DXMATRIX& view = free_fly_camera_->view_matrix();
    const D3DXVECTOR3 near_screen_space(input_.x_pos, input_.y_pos, 0);
    const D3DXVECTOR3 far_screen_space(input_.x_pos, input_.y_pos, 1);
    D3DXVec3Unproject(&near_pos, &near_screen_space, &viewport_, &proj, &view, &kMtxId);
    D3DXVec3Unproject(&far_pos, &far_screen_space, &viewport_, &proj, &view, &kMtxId);
    const D3DXVECTOR3 dir(normalize(far_pos - near_pos));

    
    for (Renderables::iterator i = renderables_.begin(), e = renderables_.end(); i != e; ++i) {
      (*i)->pick(near_pos, dir);
    }

  }

  ID3D10RenderTargetView* render_targets[1] = { resource_manager_.get_render_target("default") };
  if (render_targets[0] == NULL)  {
    LOG_WARNING_LN_ONESHOT("Unable to find render target: \"default\"");
    return true;
  }

  g_d3d_device->RSSetViewports(1, &viewport_);

  g_d3d_device->OMSetRenderTargets(1, &render_targets[0], resource_manager_.current_depth_stencil_view());
  g_d3d_device->ClearDepthStencilView( resource_manager_.current_depth_stencil_view(), D3D10_CLEAR_DEPTH, 1.0, 0 );
  g_d3d_device->ClearRenderTargetView(render_targets[0], D3DXCOLOR(0, 0, 0, 1.0f));
  

  for (Renderables::iterator i = renderables_.begin(), e = renderables_.end(); i != e; ++i) {
    (*i)->render(elapsed_time_, delta);
  }

  timer.stop();

  if (display_tweak_bar_) {
    TwDraw();
  }

  if (display_fps_) {
    calc_fps(timer.duration());
  }

  swap_chain_->Present( 0, 0 );

  last_time_ = cur_time;
  return true;
}

void System::calc_fps(const double duration)
{
  const uint32_t cur_time = timeGetTime();
  frame_count_++;
  if (last_render_ != 0) {
    if (cur_time - last_render_ > 1000) {
      const float fps = 1000 * frame_count_ / static_cast<float>(cur_time - last_render_);
      fps_str_ = to_string("Render time: %fs (%f fps)", duration, fps);
      frame_count_ = 0;
      last_render_ = cur_time;
    }
  } else {
    last_render_ = cur_time;
  }
  RECT rect;
  rect.top = 0;
  rect.bottom = WINDOW_HEIGHT;
  rect.left = 0;
  rect.right = WINDOW_WIDTH;
  font_->DrawText(NULL, fps_str_.c_str(), -1, &rect, 0, D3DXCOLOR(1, 1, 0, 1));
}

bool System::create_window() 
{
  // Note: For some weird reason, our mouse coordinates get messed up (the window knows
  // about the border, but the mosue pos doesn't), so stuff like PerfHud is a pain to
  // work with. The solution for now is to create a borderless window, since resizing 
  // doesn't work anyway.

  WNDCLASSEXA wcex;
  ZeroMemory(&wcex, sizeof(wcex));

  wcex.cbSize = sizeof(WNDCLASSEXA);
  wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc    = wnd_proc;
  wcex.hInstance      = g_instance;
  wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszClassName  = kClassName;

  if (0 == (class_atom_= RegisterClassExA(&wcex))) {
    LOG_ERROR_LN("Error registering class");
    return false;
  }

  RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  const uint32_t window_style = WS_VISIBLE | WS_POPUP/*| WS_OVERLAPPEDWINDOW*/;
//  const uint32_t window_style_ex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

  AdjustWindowRect( &rc, window_style, FALSE);
  wnd_ = CreateWindowA(kClassName, "redux 3d thing - magnus österlind - 2009", window_style,
    CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL,
    g_instance, NULL);

  return wnd_ != NULL;
}

bool System::run() 
{
  MSG msg = {0};
  while (WM_QUIT != msg.message)
  {
    if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      render();
    }
  }
  return true;
}

LRESULT CALLBACK System::wnd_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) 
{
  PAINTSTRUCT ps;
  HDC hdc;

  if (TwEventWin(hWnd, message, wParam, lParam)) {
    return 0;
  } else {
    switch( message ) {

  case WM_PAINT:
    hdc = BeginPaint( hWnd, &ps );
    g_derived_system->render(); 
    EndPaint( hWnd, &ps );
    break;

  case WM_DESTROY:
    PostQuitMessage( 0 );
    break;

  case WM_LBUTTONDOWN:
    g_derived_system->input_.left_button_down = true;
    break;

  case WM_LBUTTONUP:
    g_derived_system->input_.left_button_down = false;
    break;

  case WM_RBUTTONDOWN:
    g_derived_system->input_.right_button_down = true;
    break;

  case WM_RBUTTONUP:
    g_derived_system->input_.right_button_down  = false;
    break;

  case WM_MOUSEMOVE:
    {
      static bool first_movement = true;
      if (!first_movement) {
        g_derived_system->input_.last_x_pos = g_derived_system->input_.x_pos;
        g_derived_system->input_.last_y_pos = g_derived_system->input_.y_pos;
      }
      g_derived_system->input_.x_pos = LOWORD(lParam);
      g_derived_system->input_.y_pos = HIWORD(lParam);
      if (first_movement) {
        g_derived_system->input_.last_x_pos = g_derived_system->input_.x_pos;
        g_derived_system->input_.last_y_pos = g_derived_system->input_.y_pos;
        first_movement = false;
      }
    }
    break;

  case WM_KEYDOWN:
    g_derived_system->input_.key_status[wParam & 255] = true;
    break;

  case WM_KEYUP:
    g_derived_system->input_.key_status[wParam & 255] = false;
    switch (wParam) 
    {
    case 'F':
      g_derived_system->display_fps_ = !g_derived_system->display_fps_;
      break;

    case 'T':
      g_derived_system->display_tweak_bar_ = !g_derived_system->display_tweak_bar_;
      break;

    case VK_ESCAPE:
      PostQuitMessage( 0 );
      break;

    case VK_SPACE:
      g_derived_system->paused_ = !g_derived_system->paused_;
      break;
    }
    break;

  default:
    return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;

  }

}

bool System::init() 
{
  if (!create_window()) {
    return false;
  }

  if (!init_directx((int32_t)wnd_)) {
    return false;
  }

  init_dir_watcher();

  return true;
}

bool System::init_dir_watcher()
{
/*
  _thread_params.dir_handle = CreateFile("./", FILE_LIST_DIRECTORY, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

  if (_thread_params.dir_handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  _thread_params.watcher_completion_port = CreateIoCompletionPort(_thread_params.dir_handle, NULL, COMPLETION_KEY_IO, 0);

  if (_thread_params.watcher_completion_port == INVALID_HANDLE_VALUE) {
    return false;
  }
*/
  DWORD thread_id;
  _watcher_thread = CreateThread(0, 0, WatcherThread, (void*)&_thread_params, 0, &thread_id);

  return true;
}

bool System::close_dir_watcher()
{
  PostQueuedCompletionStatus(_thread_params.watcher_completion_port, 0, COMPLETION_KEY_SHUTDOWN, 0);
  WaitForSingleObject(_watcher_thread, INFINITE);
/*
  CloseHandle(_thread_params.watcher_completion_port);
  CloseHandle(_thread_params.dir_handle);
*/
  return true;
}


bool System::close() 
{
  close_dir_watcher();

  TwTerminate();

  ID3D10Buffer* zero_buffers[16] = { 0 };
  uint32_t strides[16] = { 0 };
  ID3D10RenderTargetView* zero_views[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT] = { 0 };
  if (g_d3d_device) {
    g_d3d_device->IASetInputLayout(NULL);
    g_d3d_device->IASetVertexBuffers(0, 16, zero_buffers, strides, strides);
    g_d3d_device->OMSetBlendState(NULL, 0, 0);
    g_d3d_device->OMSetDepthStencilState(NULL, 0);
    g_d3d_device->OMSetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, zero_views, NULL);

    g_d3d_device->VSSetShader(NULL);
    g_d3d_device->PSSetShader(NULL);

    g_d3d_device->Release();
  }
  return true;
}

void System::reset()
{
  free_fly_camera_->reset();
}

ID3D10Effect* System::create_d3d_effect(const char* hlsl, const std::string& filename) 
{
  ID3D10Effect* d3d_effect = NULL;
  ID3D10Blob* errors = NULL;
  if (FAILED(D3DX10CreateEffectFromMemory(
    hlsl, strlen(hlsl), ("effects\\" + filename).c_str(), NULL, NULL, "fx_4_0", 
    D3D10_SHADER_ENABLE_STRICTNESS|D3D10_SHADER_OPTIMIZATION_LEVEL3, 
    D3D10_EFFECT_SINGLE_THREADED, g_d3d_device, NULL, NULL, &d3d_effect, &errors, NULL))) {
      if (errors != NULL) {
        if (void* buffer = errors->GetBufferPointer()) {
          LOG_ERROR_LN("%s", buffer);
          throw std::exception((const char*)buffer);
        }
      }
      throw std::exception("Error loading fx file");
  }
  return d3d_effect;
}

Handle System::load_effect(const std::string& filename) 
{
  std::string path("effects/" + filename + ".fx");
  uint8_t* hlsl = NULL;
  uint32_t len = 0;
  if (!load_file(hlsl, len, path.c_str(), true)) {
    LOG_ERROR_LN("Error loading effect: %s", filename.c_str());
    return Handle();
  }
  ID3D10Effect* d3d_effect = create_d3d_effect((const char*)hlsl, filename);
  NameToTechnique techs = collect_techniques(d3d_effect);

  std::string hlsl_str((const char*)hlsl, len);
  SAFE_ADELETE(hlsl);

  return handle_manager_.add(new Effect(NULL, hlsl_str, d3d_effect, techs));
}


Handle System::create_vertex_buffer(uint8_t* data, const uint32_t vertex_count, const uint32_t vertex_size) 
{
  D3D10_BUFFER_DESC buffer_desc;
  ZeroMemory(&buffer_desc, sizeof(buffer_desc));
  buffer_desc.Usage     = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = vertex_count * vertex_size;
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;

  D3D10_SUBRESOURCE_DATA init_data;
  ZeroMemory(&init_data, sizeof(init_data));
  init_data.pSysMem = data;
  ID3D10Buffer* vertex_buffer = NULL;
  ENFORCE_HR(g_d3d_device->CreateBuffer(&buffer_desc, &init_data, &vertex_buffer));
  return handle_manager_.add(new Buffer(vertex_buffer, vertex_count, vertex_size));
}

Handle System::create_index_buffer(uint8_t* data, const uint32_t index_count, const uint32_t index_size) 
{
  D3D10_BUFFER_DESC buffer_desc;
  ZeroMemory(&buffer_desc, sizeof(buffer_desc));
  buffer_desc.Usage     = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = index_count * index_size;
  buffer_desc.BindFlags = D3D10_BIND_INDEX_BUFFER;

  D3D10_SUBRESOURCE_DATA init_data;
  ZeroMemory(&init_data, sizeof(init_data));
  init_data.pSysMem = data;
  ID3D10Buffer* index_buffer = NULL;
  ENFORCE_HR(g_d3d_device->CreateBuffer(&buffer_desc, &init_data, &index_buffer));
  return handle_manager_.add(new Buffer(index_buffer, index_count, index_size));
}

Handle System::create_input_layout(D3D10_INPUT_ELEMENT_DESC* descs, const uint32_t num_descs, const D3D10_PASS_DESC& pass_desc) 
{
  ID3D10InputLayout* layout = NULL;
  ENFORCE_HR(g_d3d_device->CreateInputLayout( 
    descs, num_descs, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &layout));
  return handle_manager_.add(new InputLayout(layout));
}

void System::set_input_layout(const Handle handle) 
{
  InputLayout* layout = NULL;
  if (handle_manager_.get(handle, layout)) {
    g_d3d_device->IASetInputLayout(layout->layout);
  }
}

void System::add_renderable(Renderable* renderable) 
{
  renderables_.push_back(renderable);
}

bool System::get_technique_pass_desc(const Handle effect_handle, const std::string& technique_name, D3D10_PASS_DESC& desc)
{
  Effect* effect = NULL;
  if (handle_manager_.get(effect_handle, effect)) {
    if (ID3D10EffectTechnique* technique = effect->effect->GetTechniqueByName(technique_name.c_str())) {
      if (ID3D10EffectPass* pass = technique->GetPassByIndex(0)) {
        pass->GetDesc(&desc);
        return true;
      }
    }
  }
  LOG_WARNING_LN("Unable to find desc for technique: %s", technique_name.c_str());
  return false;
}

void System::set_render_targets(const RenderTargets& /*render_targets*/) 
{
  typedef std::vector<ID3D10RenderTargetView*> RenderTargetViews;
  RenderTargetViews rt;
}

void System::set_vertex_buffer(const Handle& vb) 
{
  Buffer* buf = NULL;
  handle_manager_.get(vb, buf);
  const UINT offset = 0;
  g_d3d_device->IASetVertexBuffers(0, 1, &buf->buffer, &buf->size, &offset);
}

void System::set_index_buffer(const Handle& ib) 
{
  Buffer* buf = NULL;
  handle_manager_.get(ib, buf);
  g_d3d_device->IASetIndexBuffer(buf->buffer, buf->size == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
}

void System::set_primitive_topology(const D3D10_PRIMITIVE_TOPOLOGY top) 
{
  g_d3d_device->IASetPrimitiveTopology(top);
}

void System::draw_indexed(const uint32_t index_count, const uint32_t start_index_location, const uint32_t base_vertex_location)
{
  g_d3d_device->DrawIndexed(index_count, start_index_location, base_vertex_location);
}

void System::set_active_effect(LPD3D10EFFECT effect)
{
  active_effect_ = effect;
}

void System::set_active_effect(const Handle /*handle*/) 
{
}

void System::set_active_technique(const std::string& name)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectTechnique* technique = active_effect_->GetTechniqueByName(name.c_str()) ) {
    technique->GetPassByIndex(0)->Apply(0);
  }
}

// Hehe, I wonder how many times I'm to look at this ugly list of repititive crap and think to myself
// "If only I coded Lisp instead" :P (TODO: rewrite as a #define)
void System::set_variable(const std::string& name, const float value)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectScalarVariable* typed_variable = effect_variable->AsScalar()) {
      if (SUCCEEDED(typed_variable->SetFloat(value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_variable(const std::string& name, const D3DXVECTOR2& value)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
      if (SUCCEEDED(typed_variable->SetFloatVector((float*)&value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_variable(const std::string& name, const D3DXVECTOR3& value)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
      if (SUCCEEDED(typed_variable->SetFloatVector((float*)&value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_variable(const std::string& name, const D3DXVECTOR4& value)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
      if (SUCCEEDED(typed_variable->SetFloatVector((float*)&value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_variable(const std::string& name, const D3DXCOLOR& value)
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
      if (SUCCEEDED(typed_variable->SetFloatVector((float*)&value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_variable(const std::string& name, const D3DXMATRIX& value) 
{
  SUPER_ASSERT(active_effect_ != NULL);
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectMatrixVariable* typed_variable = effect_variable->AsMatrix()) {
      if (SUCCEEDED(typed_variable->SetMatrix((float*)&value))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting variable: %s", name.c_str());
}

void System::set_resource(const std::string& name, const std::string& resource_name)
{
  SUPER_ASSERT(active_effect_ != NULL);
  Textures::iterator it = textures_.find(resource_name);
  if (it == textures_.end()) {
    LOG_WARNING_LN("Unable to find texture: %s", name.c_str());
    return;
  }
  if (ID3D10EffectVariable* effect_variable = active_effect_->GetVariableByName(name.c_str())) {
    if (ID3D10EffectShaderResourceVariable* typed_variable = effect_variable->AsShaderResource()) {
      if (SUCCEEDED(typed_variable->SetResource(it->second))) {
        return;
      }
    }
  }
  LOG_WARNING_LN("Error setting resource: %s", name.c_str());
}

void System::load_texture(const std::string& name)
{
  ID3D10ShaderResourceView* texture = NULL;

  D3DX10_IMAGE_LOAD_INFO loadInfo;
  ZeroMemory( &loadInfo, sizeof(D3DX10_IMAGE_LOAD_INFO) );
  loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  loadInfo.Format = DXGI_FORMAT_BC1_UNORM;

  // hack a path to the texture
  const string texture_path("data\\textures\\" + name);

  if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_d3d_device, texture_path.c_str(), &loadInfo, NULL, &texture, NULL )) ){
    LOG_WARNING_LN("Error loading texture: %s", texture_path.c_str());
    return;
  }

  textures_.insert(make_pair(name, texture));
}

void System::get_free_fly_camera(D3DXVECTOR3& eye_pos, D3DXMATRIX& view_mtx)
{
  eye_pos = free_fly_camera_->eye_pos();
  view_mtx = free_fly_camera_->view_matrix();
}

ID3D10Device* System::get_device()
{
  return g_d3d_device;
}

void System::key_down(const uint32_t key)
{
  if (key <= 256) {
    input_.key_status[key] = true;
  }
}

void System::key_up(const uint32_t key)
{
  if (key <= 256) {
    input_.key_status[key] = false;
  }
}

void System::mouse_click(const bool left_button_down, const bool right_button_down)
{
  input_.left_button_down = left_button_down;
  input_.right_button_down = right_button_down;
}

void System::mouse_move(const int x_pos, const int x_last, const int y_pos, const int y_last)
{
  input_.x_pos = (uint16_t)x_pos;
  input_.last_x_pos = (uint16_t)x_last;
  input_.y_pos = (uint16_t)y_pos;
  input_.last_y_pos = (uint16_t)y_last;
}

void System::add_process_input_callback(const ProcessInputCallback& cb)
{
  process_input_callbacks.push_back(cb);
}

void System::add_event_handler(const EventId::Enum id, const EventHandler& handler)
{
  _event_handlers.insert(std::make_pair(id, handler));
}

void System::process_deferred_events()
{
  // lock
  EnterCriticalSection(&_cs_event);
  for (DeferredEvents::iterator i = _deferred_events.begin(), e = _deferred_events.end(); i != e; ++i) {
    const EventId::Enum id = i->first;
    const EventArgs* args = i->second;
    const EventHandlerRange &range = _event_handlers.equal_range(id);

    for (EventHandlers::iterator i = range.first, e = range.second; i != e; ++i) {
      i->second(args);
    }
    delete args;
  }

  _deferred_events.clear();
  LeaveCriticalSection(&_cs_event);
}

void System::send_event(const EventId::Enum id, const EventArgs* event_args)
{
  // only send events from the main thread. other events are stored on the deferred list
  if (GetCurrentThreadId() == _main_thread_id) {
    const EventHandlerRange &range = _event_handlers.equal_range(id);

    for (EventHandlers::iterator i = range.first, e = range.second; i != e; ++i) {
      (i->second)(event_args);
    }
    delete event_args;
  } else {
    // lock
    EnterCriticalSection(&_cs_event);
    _deferred_events.push_back(std::make_pair(id, event_args));
    LeaveCriticalSection(&_cs_event);
  }
}

void System::remove_event_handler(const EventId::Enum /*id*/, const EventHandler& /*handler*/)
{

}

void System::watch_file(const char* filename, const FileChangedCallback &cb)
{
  _file_changed_callbacks.insert(std::make_pair(std::string(filename), cb));
}

void System::unwatch_file(const char* /*filename*/, const FileChangedCallback &/*cb*/)
{
  // todo
}

void System::file_changed(const EventArgs* args)
{
  const FileChangedEventArgs *ea = static_cast<const FileChangedEventArgs*>(args); 
  const FileChangedCallbackskRange &range = _file_changed_callbacks.equal_range(std::string(ea->filename));

  for (FileChangedCallbacks::iterator i = range.first, e = range.second; i != e; ++i) {
    (i->second)();
  }

}

extern "C"
{
  __declspec( dllexport ) SystemInterface* create_system()
  {
    return new System();
  }
}
