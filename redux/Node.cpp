#include "StdAfx.h"
#include "Node.hpp"

Node::Node(void) 
{
  D3DXMatrixIdentity(&world_matrix_);
}

Node::~Node(void) 
{
}
