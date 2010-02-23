#ifndef TENTACLE_HPP
#define TENTACLE_HPP

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Utils.hpp"

struct Tentacle
{
  D3DXVECTOR3 prev_tangent;

  typedef VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> > VB;
  typedef IndexBuffer<uint32_t> IB;

  void init(VB* vb, IB* ib, const ID3D10DevicePtr& device)
  {
    _vb = vb;
    _ib = ib;
    _device = device;
    splits_ = 20;
    start_time_ = 0;

    control_points_.push_back(D3DXVECTOR3(0,0,0));
    control_points_.push_back(D3DXVECTOR3(0,5,0));
  }

  void render(const uint32_t time)
  {
    if (start_time_ == 0) {
      start_time_ = time;
    }

    const uint32_t growth_rate = 100;

    if ((time - start_time_) / growth_rate > control_points_.size()) {
      //const float angle = randf((float)-D3DX_PI/6, (float)D3DX_PI/6);
      D3DXVECTOR3 tmp_tangent(prev_tangent);
      D3DXVec3Normalize(&tmp_tangent, &tmp_tangent);
      //D3DXVECTOR3 new_point(cosf(angle)/tmp_tangent);
      D3DXVECTOR3 new_point(0,0,0);

      control_points_.push_back(control_points_.back() + new_point);
    }

    _vb->start_frame();

    float inc = 1 / (float)(splits_ - 1);

    const uint32_t spins = 20;

    bool first_point = true;

    D3DXVECTOR3 binormal;
    D3DXVECTOR3 prev_binormal;
    D3DXVECTOR3 tangent;
    D3DXVECTOR3 normal;
    D3DXVECTOR3 prev_normal;

    const int32_t num_knots = control_points_.size();
    for (int32_t i = 0; i < num_knots-1; ++i) {
      const bool last_knot = i == num_knots - 2;
      const D3DXVECTOR3 p_1 = control_points_[max(0,i-1)];
      const D3DXVECTOR3 p0 = control_points_[i+0];
      const D3DXVECTOR3 p1 = control_points_[min(num_knots-1, i+1)];
      const D3DXVECTOR3 p2 = control_points_[min(num_knots-1, i+2)];

      D3DXVECTOR3 m0 = (p1 - p_1) / 2;
      D3DXVECTOR3 m1 = (p2 - p0) / 2;

      float cur_t = 0;
      for (uint32_t j = 0; j < splits_; ++j, cur_t += inc) {
        D3DXVECTOR3 cur;
        D3DXVec3Hermite(&cur, &p0, &m0, &p1, &m1, cur_t);
        cur = catmull(cur_t, p_1, p0, p1, p2);
        tangent = catmull_vel(cur_t, p_1, p0, p1, p2);
        D3DXVec3Normalize(&tangent, &tangent);

        if (first_point) {
          D3DXVECTOR3 q = catmull_acc(cur_t, p_1, p0, p1, p2);
          D3DXVec3Cross(&normal, &tangent, &q);
          if (D3DXVec3LengthSq(&normal) == 0) {
            // if the curvature is 0, choose any normal perpedictular to the tangent
            normal = find_orthogonal(tangent);
          } else {
            D3DXVec3Cross(&normal, &normal, &tangent);
          }
          D3DXVec3Normalize(&normal, &normal);

          D3DXVec3Cross(&binormal, &tangent, &normal);
          first_point = false;
        } else {
          // Ken Sloan's method to propagate the reference frame
          D3DXVec3Cross(&normal, &prev_binormal, &tangent);
          D3DXVec3Cross(&binormal, &tangent, &normal);
        }

        prev_binormal = binormal;
        prev_tangent = tangent;


        const float delta_angle = 2 * (float)D3DX_PI / (float)(spins - 1);
        float cur_angle = 0;
        D3DXVECTOR3 tmp;
        D3DXVECTOR3 tmp2;
        D3DXMATRIX mtx_rot2(kMtxId);

        D3DXVECTOR3 scaled_s = normal;
        if (last_knot) {
          scaled_s *= ((splits_-1) - j) / (float)splits_;
        }

        for (uint32_t k = 0; k < spins; ++k, cur_angle += delta_angle) {
          D3DXMatrixRotationAxis(&mtx_rot2, &tangent, cur_angle);
          D3DXVec3TransformCoord(&tmp, &scaled_s, &mtx_rot2);
          D3DXVec3TransformNormal(&tmp2, &normal, &mtx_rot2);

          _vb->add(tmp + cur, tmp2);
        }
      }
    }

    _ib->start_frame();
    for (uint32_t i = 0; i < _vb->vertex_count() / spins  - 1; ++i) {
      for (uint32_t j = 0; j < spins; ++j) {

        // d-c
        // | |
        // b-a
        const uint32_t next_j = (j + 1) % spins;
        const uint32_t a = i*spins + j;
        const uint32_t b = i*spins + next_j;
        const uint32_t c = (i+1)*spins + j;
        const uint32_t d = (i+1)*spins + next_j;

        // a, b, c
        _ib->add(a);
        _ib->add(b);
        _ib->add(c);

        // b, d, c
        _ib->add(b);
        _ib->add(d);
        _ib->add(c);
      }
    }
    _ib->end_frame();

    _vb->end_frame();
    _vb->set_input_layout();
    _vb->set_vertex_buffer();
    _ib->set_index_buffer();
    _device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _device->DrawIndexed(_ib->index_count(), 0, 0);
  }

  VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> > *_vb;
  IndexBuffer<uint32_t> *_ib;

  ID3D10DevicePtr _device;
  
  typedef std::vector<D3DXVECTOR3> ControlPoints;
  ControlPoints control_points_;
  uint32_t splits_;
  uint32_t start_time_;
};


#endif