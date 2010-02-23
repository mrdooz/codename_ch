#ifndef ANIMATION_NODE_HPP
#define ANIMATION_NODE_HPP

#include "ReduxTypes.hpp"

struct AnimationKey
{
  AnimationKey() : time_in_ms(0xffffffff) {}
  AnimationKey(const uint32_t time_in_ms, const D3DXVECTOR3& pos, const D3DXQUATERNION& rot, const D3DXVECTOR3& scale) 
    : time_in_ms(time_in_ms), pos(pos), rot(rot), scale(scale) {}
  uint32_t time_in_ms;
  D3DXVECTOR3 pos;
  D3DXQUATERNION rot;
  D3DXVECTOR3 scale;
};

typedef std::vector<AnimationKey> AnimationKeys;

class AnimationNode
{
  friend class ReduxLoader;
  friend class AnimationManager;
public:
  AnimationNode(const std::string& name);
  ~AnimationNode();

  const std::string& name() { return name_; }
  const AnimationKeys& keys() const { return keys_; }
  D3DXMATRIX transform() const { return transform_; }
  void set_transform(const D3DXMATRIX& mtx);
  void pos_rot_scale(D3DXVECTOR3& pos, D3DXQUATERNION& rot, D3DXVECTOR3& scale) const;
  void set_pos_rot_scale(const D3DXVECTOR3& pos, const D3DXQUATERNION& rot, const D3DXVECTOR3& scale);

  const uint32_t num_children() const { return children_.size(); }
  AnimationNodeSPtr child(const uint32_t idx) { return children_[idx]; }
  //AnimationNodeSPtr find_node_by_name(const std::string& node_name);
private:
  D3DXVECTOR3 pos_;
  D3DXQUATERNION rot_;
  D3DXVECTOR3 scale_;
  D3DXMATRIX transform_;
  std::string name_;
  std::vector<AnimationNodeSPtr> children_;
  AnimationKeys keys_;
};

#endif // #ifndef ANIMATION_NODE_HPP
