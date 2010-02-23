#ifndef REDUX_LOADER_HPP
#define REDUX_LOADER_HPP

#include "ReduxTypes.hpp"

class ChunkIo;
class MaterialManager;
struct SystemInterface;
struct Scene;
class AnimationNode;

class ReduxLoader
{
public:
  ReduxLoader(const std::string& filename, Scene* scene, SystemInterface* system, AnimationManager* animation_manager);
  void load();
private:
  void load_camera(ChunkIo& reader);
  void  load_mesh(ChunkIo& reader);
  void  load_hierarchy_inner(ChunkIo& reader, AnimationNode* parent);
  void  load_hierarchy(ChunkIo& reader);
  void  load_animation(ChunkIo& reader);

  std::string filename_;
  Scene* scene_;
  SystemInterface* system_;
  AnimationManager* animation_manager_;
};

#endif // #ifndef REDUX_LOADER_HPP
