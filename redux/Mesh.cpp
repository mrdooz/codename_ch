#include "stdafx.h"
#include "Mesh.hpp"
#include <celsus/Logger.hpp>


Mesh::Mesh(const std::string& name) 
  : name_(name)
  , index_count_(0)
  , vertex_buffer_stride_(0)
  , vertex_buffer_(NULL)
  , input_layout2_(NULL)
{
  D3DXMATRIX world_matrix;
  D3DXMatrixIdentity(&world_matrix);
}

Mesh::~Mesh() 
{
  SAFE_RELEASE(vertex_buffer_);
  SAFE_RELEASE(index_buffer_);
  SAFE_RELEASE(input_layout2_);

  FOREACH(D3D10_INPUT_ELEMENT_DESC desc, input_element_descs_) {
    free((void*)(desc.SemanticName));
  }
}

void Mesh::create_input_layout(SystemInterface& system, const D3D10_PASS_DESC& pass_desc) 
{
  input_layout_ = system.create_input_layout(&input_element_descs_[0], input_element_descs_.size(), pass_desc);
}

void Mesh::set_input_layout(ID3D10InputLayout* layout)
{
  input_layout2_ = layout;
}
