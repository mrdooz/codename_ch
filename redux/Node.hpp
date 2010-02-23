#pragma once

/**
 *	Base class for everything that has a transformation matrix
 */ 
class Node
{
public:
  Node(void);
  virtual ~Node(void);

  const D3DXMATRIX& world_matrix() const { return world_matrix_; }
  const std::string& name() { return name_; }

private:
  friend class ReduxLoader;
  D3DXMATRIX   world_matrix_;
  std::string   name_;
};
