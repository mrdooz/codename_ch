#include "stdafx.h"
#include <celsus/celsus.hpp>
#include "AnimationManager.hpp"

namespace {
  D3DXMATRIX matrix_from_key(const AnimationKey& key)
  {
    D3DXMATRIX mtx;
    D3DXMatrixTransformation(&mtx, &kVec3Zero, &kQuatId, &key.scale, &kVec3Zero, &key.rot, &key.pos);
    return mtx;
  }
}

using namespace std;


AnimationManager::AnimationManager()
  : fps_(0)
  , start_time_(0)
  , end_time_(0)
  , loop_animations_(true)
{
}

AnimationManager::~AnimationManager()
{
  root_.clear();
}

/**
 * Get the pair of keys that we want to interpolate between, along with the ratio. Returns true if we find
 * a pair, otherwise next is invalid, and we should use the value in cur.
 */ 
bool AnimationManager::keys_at_time(
  AnimationKey& cur, AnimationKey& next, float& ratio, const AnimationKeys& keys, const uint32_t time_in_ms)
{
  // Node found, so look for key frame
  const uint32_t num_keys = keys.size();

  if (time_in_ms <= keys.front().time_in_ms) {
    cur = keys.front();
    return false;
  }

  if (time_in_ms >= keys.back().time_in_ms) {
    cur = keys.back();
    return false;
  }

  for (uint32_t i = 0; i < num_keys-1; ++i) {
    if (time_in_ms >= keys[i].time_in_ms && time_in_ms < keys[i+1].time_in_ms) {
      // Found it, so we interpolate the values
      cur = keys[i];
      next = keys[i+1];
      const float delta = (float)(next.time_in_ms - cur.time_in_ms);
      ratio = (time_in_ms - cur.time_in_ms) / delta;
      return true;
    }
  }

  // We should never get here..
  assert(false);
  return false;
}

const AnimationKeys& AnimationManager::get_keys_for_node(const std::string& node_name) const
{
  AnimationNodeSPtr node = find_node_by_name(node_name);
  SUPER_ASSERT(node);
  return node->keys();
}

void AnimationManager::get_transform_for_node(D3DXMATRIX& mtx, const std::string& node_name)
{
  if (AnimationNodeSPtr node = find_node_by_name(node_name)) {
    mtx = node->transform();
  } else {
    LOG_WARNING_LN_ONESHOT("[%s] Node: %s not found", __FUNCTION__, node_name.c_str());
    mtx = kMtxId;
  }
}

void AnimationManager::get_transform_for_node(D3DXVECTOR3& pos, const std::string& node_name)
{
  if (AnimationNodeSPtr node = find_node_by_name(node_name)) {
    D3DXVECTOR3 scale;
    D3DXQUATERNION rot;
    node->pos_rot_scale(pos, rot, scale);
  } else {
    LOG_WARNING_LN_ONESHOT("[%s] Node: %s not found", __FUNCTION__, node_name.c_str());
    pos = kVec3Zero;
  }
}

void AnimationManager::get_transform_for_node(D3DXVECTOR3& pos, D3DXQUATERNION& rot, D3DXVECTOR3& scale, const std::string& node_name)
{
  if (AnimationNodeSPtr node = find_node_by_name(node_name)) {
    node->pos_rot_scale(pos, rot, scale);
  } else {
    LOG_WARNING_LN_ONESHOT("[%s] Node: %s not found", __FUNCTION__, node_name.c_str());
    pos = kVec3Zero;
    rot = kQuatId;
    scale = kVec3One;
  }
}

void AnimationManager::calc_transform_at_time(D3DXMATRIX& mtx, const uint32_t time_in_ms, AnimationNode* node)
{
  const AnimationKeys& keys = node->keys();

  switch( keys.size() ) {
    case 0:
      mtx = kMtxId;
      break;

    case 1:
      mtx = matrix_from_key(keys[0]);
      break;

    default:
      {
        AnimationKey cur, next;
        float ratio;
        if (!keys_at_time(cur, next, ratio, keys, time_in_ms)) {
          mtx = matrix_from_key(cur);
          return;
        }

        const D3DXMATRIX mtx_cur = matrix_from_key(cur);
        const D3DXMATRIX mtx_next = matrix_from_key(next);
        mtx = mtx_cur + ratio * (mtx_next - mtx_cur);
      }
  }
}

void AnimationManager::calc_transform_at_time(D3DXMATRIX& mtx, const uint32_t time_in_ms, const std::string& name)
{
  return calc_transform_at_time(mtx, time_in_ms, find_node_by_name(name).get());
}

uint32_t AnimationManager::get_looped_time(const uint32_t time_in_ms) const
{
  if (end_time_ == 0) {
    return 0;
  }

  if (!loop_animations_) {
    return time_in_ms;
  }
  return (time_in_ms % end_time_);
}

AnimationNodeSPtr AnimationManager::find_node_by_name_inner(const std::string& node_name, const AnimationNodeSPtr& cur) const
{
  if (cur->name() == node_name) {
    return cur;
  }

  for (uint32_t i = 0; i < cur->num_children(); ++i) {
    if (AnimationNodeSPtr result = find_node_by_name_inner(node_name, cur->child(i))) {
      return result;
    }
  }

  return AnimationNodeSPtr();
}

AnimationNodeSPtr AnimationManager::find_node_by_name(const std::string& node_name) const
{
  for (uint32_t i = 0; i < root_.size(); ++i) {
    if (AnimationNodeSPtr result = find_node_by_name_inner(node_name, root_[i])) {
      return result;
    }
  }
  return AnimationNodeSPtr();
}

void AnimationManager::update_transforms_inner(const uint32_t time, AnimationNode* node, const D3DXMATRIX& parent_transform)
{
  D3DXMATRIX transform;
  calc_transform_at_time(transform, time, node);
  node->set_transform(transform);
  for (uint32_t i = 0; i < node->num_children(); ++i) {
    update_transforms_inner(time, node->child(i).get(), transform);
  }
}

void AnimationManager::update_transforms(const uint32_t time)
{
  const uint32_t local_time = get_looped_time(time);

  for (uint32_t i = 0; i < root_.size(); ++i) {
    update_transforms_inner(local_time, root_[i].get(), kMtxId);
  }
}
