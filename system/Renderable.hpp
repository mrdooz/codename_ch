#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

struct Renderable 
{
  virtual ~Renderable() {};
  virtual bool init() { return true; }
  virtual bool close() { return true; }
  virtual void render(const int32_t time_in_ms, const int32_t delta) = 0;
  virtual void pick(const D3DXVECTOR3& /*org*/, const D3DXVECTOR3& /*dir*/) {}
};

#endif