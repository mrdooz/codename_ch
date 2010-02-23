#ifndef SPRING_HPP
#define SPRING_HPP

#include "../system/Renderable.hpp"
#include "../system/Serializer.hpp"

struct SystemInterface;
class EffectManager;
class DebugRenderer;

typedef boost::shared_ptr<SystemInterface> SystemSPtr;
typedef boost::shared_ptr<EffectManager> EffectManagerSPtr;

struct Point;
struct Force;

typedef boost::shared_ptr<Point> PointPtr;
typedef std::vector<PointPtr> Points;
typedef std::vector<Force*> Forces;

class SpringTest : public Renderable
{
  friend class ReduxLoader;
public:
  SpringTest(const SystemSPtr& system, const EffectManagerSPtr& effect_manager);
  ~SpringTest();
  virtual bool init();
  virtual bool close();
  virtual void render(const int32_t time_in_ms, const int32_t delta);
  virtual void pick(const D3DXVECTOR3& org, const D3DXVECTOR3& dir);
private:

  void  calc_forces();
  void  apply_forces();

  Points points_;
  Forces forces_;

  int32_t last_update_;

  SystemSPtr system_;
  EffectManagerSPtr effect_manager_;
  DebugRenderer* debug_renderer_;

  D3DXVECTOR3 ray_org_;
  D3DXVECTOR3 ray_dir_;

  SERIALIZE( SpringTest, 
    MEMBER(ray_org_) MEMBER(ray_dir_));

};

#endif
