#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include "AnimationNode.hpp"
#include "ReduxTypes.hpp"

class AnimationManager 
{
  friend class ReduxLoader;
public:
  AnimationManager();
  ~AnimationManager();

  void add_animation_keys(const std::string& name, const AnimationKeys& keys);
  void update_transforms(const uint32_t time);

  void get_transform_for_node(D3DXMATRIX& mtx, const std::string& node_name);
  void get_transform_for_node(D3DXVECTOR3& pos, const std::string& node_name);
  void get_transform_for_node(D3DXVECTOR3& pos, D3DXQUATERNION& rot, D3DXVECTOR3& scale, const std::string& node_name);

private:
  AnimationNodeSPtr find_node_by_name(const std::string& node_name) const;
  AnimationNodeSPtr find_node_by_name_inner(const std::string& node_name, const AnimationNodeSPtr& cur) const;
  void calc_transform_at_time(D3DXMATRIX& mtx, const uint32_t time_in_ms, const std::string& name);
  void calc_transform_at_time(D3DXMATRIX& mtx, const uint32_t time_in_ms, AnimationNode* node);
  void update_transforms_inner(const uint32_t time, AnimationNode* node, const D3DXMATRIX& parent_transform);

  const AnimationKeys& get_keys_for_node(const std::string& node_name) const;
  uint32_t get_looped_time(const uint32_t time_in_ms) const;
  bool keys_at_time(AnimationKey& cur, AnimationKey& next, float& delta, const AnimationKeys& keys, const uint32_t time_in_ms);

  std::vector<AnimationNodeSPtr> root_;
  bool loop_animations_;
  uint32_t fps_;
  uint32_t start_time_;
  uint32_t end_time_;
};

#endif // #ifndef ANIMATION_MANAGER_HPP
