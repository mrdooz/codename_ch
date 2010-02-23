#ifndef M2LOADER_HPP
#define M2LOADER_HPP

struct Scene;

class M2Loader
{
public:
  M2Loader();
  void  load(const char* filename, Scene* scene);

private:

  Scene* scene_;

};

#endif
