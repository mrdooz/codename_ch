#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <boost/tuple/tuple.hpp>
#include <hash_map>

class ResourceManager
{
public:
  ResourceManager();
  ~ResourceManager(void);

  void  add_lua_script(const std::string& filename);

  void  set_current_render_target(const std::string& name);
  void  set_current_depth_stencil(const std::string& name);

  ID3D10DepthStencilView* current_depth_stencil_view() { return current_depth_stencil_view_; }
  ID3D10RenderTargetView* current_render_target_view() { return current_render_target_view_; }

  void  add_render_target_view(ID3D10RenderTargetView* view, const std::string& name);
  void  add_render_target_view(ID3D10RenderTargetView* view, ID3D10ShaderResourceView* resource, const std::string& name);
  void  add_depth_stencil_view(ID3D10DepthStencilView* view, const std::string& name);

  ID3D10RenderTargetView* get_render_target(const std::string& name);
  ID3D10ShaderResourceView* get_shader_resource_view(const std::string& name);
  ID3D10DepthStencilView* get_depth_stencil_view(const std::string& name);

/*
  boost::tuple<bool, ID3D10RenderTargetView*> get_render_target(const std::string& name);
  boost::tuple<bool, ID3D10ShaderResourceView*> get_shader_resource_view(const std::string& name);
  boost::tuple<bool, ID3D10DepthStencilView*> get_depth_stencil_view(const std::string& name);
*/
private:
  struct RenderTargetData 
  {
    RenderTargetData(ID3D10RenderTargetView* rt, ID3D10ShaderResourceView* sr) 
      : render_target_view(rt), shader_resource_view(sr) {}
    ID3D10RenderTargetView* render_target_view;
    ID3D10ShaderResourceView* shader_resource_view;
  };

  typedef std::string RenderTargetName;
  typedef std::string DepthStencilName;

  typedef stdext::hash_map<RenderTargetName, RenderTargetData> NameToRenderTarget;
  typedef stdext::hash_map<DepthStencilName, ID3D10DepthStencilView*> NameToDepthStencil;

  struct DirectoryChange 
  {
    std::string dir_name_;
    HANDLE dir_handle_;
  };

  NameToRenderTarget  name_to_render_target_;
  NameToDepthStencil  name_to_depth_stencil_;

  ID3D10RenderTargetView* current_render_target_view_;
  ID3D10DepthStencilView* current_depth_stencil_view_;
};

#endif // #ifndef RESOURCE_MANAGER_HPP
