#include "StdAfx.h"
#include "AnimationNode.hpp"

AnimationNode::AnimationNode(const std::string& name) 
  : name_(name) 
  , pos_(kVec3Zero)
  , rot_(kQuatId)
  , scale_(kVec3One)
  , transform_(kMtxId)
{
}

AnimationNode::~AnimationNode()
{
  children_.clear();
}
/*
AnimationNodeSPtr AnimationNode::find_node_by_name(const std::string& node_name)
{
  if (node_name == name_) {
    return this;
  }

  for (uint32_t i = 0; i < children_.size(); ++i) {
    if (AnimationNode* result = children_[i]->find_node_by_name(node_name)) {
      return result;
    }
  }

  return NULL;
}
*/
void AnimationNode::pos_rot_scale(D3DXVECTOR3& pos, D3DXQUATERNION& rot, D3DXVECTOR3& scale) const
{
  pos = pos_;
  rot = rot_;
  scale = scale_;
}

void AnimationNode::set_pos_rot_scale(const D3DXVECTOR3& pos, const D3DXQUATERNION& rot, const D3DXVECTOR3& scale)
{
  pos_ = pos;
  rot_ = rot;
  scale_ = scale;
  D3DXMatrixTransformation(&transform_, &kVec3Zero, &kQuatId, &scale, &kVec3Zero, &rot, &pos);
}

void AnimationNode::set_transform(const D3DXMATRIX& transform)
{
  transform_ = transform;
  pos_ = get_translation(transform);
  D3DXQuaternionRotationMatrix(&rot_, &transform);
  scale_ = get_scale(transform);
}
