#include "StdAfx.h"
#include "ResourceManager.hpp"


ResourceManager::ResourceManager()
  : current_render_target_view_(NULL)
  , current_depth_stencil_view_(NULL)
{
}

ResourceManager::~ResourceManager(void)
{
}

void ResourceManager::set_current_render_target(const std::string& name) 
{
  RenderTargetData rt = safe_find_value(name_to_render_target_, name);
  current_render_target_view_ = rt.render_target_view;
}

void ResourceManager::set_current_depth_stencil(const std::string& name) 
{
  current_depth_stencil_view_ = safe_find_value(name_to_depth_stencil_, name);
}

void ResourceManager::add_render_target_view(ID3D10RenderTargetView* view, const std::string& name) 
{
  ENFORCE(!exists_in_container(name_to_render_target_, name))(name);
  name_to_render_target_.insert(make_pair(name, RenderTargetData(view, NULL)));
}

void ResourceManager::add_render_target_view(ID3D10RenderTargetView* view, ID3D10ShaderResourceView* resource, const std::string& name) 
{
  ENFORCE(!exists_in_container(name_to_render_target_, name))(name);
  name_to_render_target_.insert(make_pair(name, RenderTargetData(view, resource)));
}

void ResourceManager::add_depth_stencil_view(ID3D10DepthStencilView* view, const std::string& name) 
{
  ENFORCE(!exists_in_container(name_to_depth_stencil_, name))(name);
  name_to_depth_stencil_.insert(make_pair(name, view));
}

ID3D10RenderTargetView* ResourceManager::get_render_target(const std::string& name) 
{
  NameToRenderTarget::iterator it = name_to_render_target_.find(name);
  return it == name_to_render_target_.end() ? NULL : it->second.render_target_view;
}

ID3D10DepthStencilView* ResourceManager::get_depth_stencil_view(const std::string& name) 
{
  NameToDepthStencil::iterator it = name_to_depth_stencil_.find(name);
  return it == name_to_depth_stencil_.end() ? NULL : it->second;
}

ID3D10ShaderResourceView* ResourceManager::get_shader_resource_view(const std::string& name) 
{
  NameToRenderTarget::iterator it = name_to_render_target_.find(name);
  return it == name_to_render_target_.end() ? NULL : it->second.shader_resource_view;
}
