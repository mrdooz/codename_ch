#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

namespace mpl = boost::mpl;

template<class TList>
struct VertexBuffer
{
  ID3D10BufferPtr vb_;
  ID3D10DevicePtr device_;
  ID3D10InputLayoutPtr input_layout_;
  uint8_t* double_buffer_;
  uint32_t data_ofs_;
  uint32_t buffer_size_;
  uint32_t vertex_count_;
  bool resized_;

  enum { VertexSize = mpl::accumulate<
    TList,
    mpl::int_<0>,
    mpl::plus<mpl::_1, mpl::sizeof_<mpl::_2> > >::type::value };

  void set_input_layout()
  {
    device_->IASetInputLayout(input_layout_);

  }

  uint32_t vertex_count() const { return vertex_count_; }

  VertexBuffer(const ID3D10DevicePtr& device)
    : device_(device)
    , double_buffer_(NULL)
    , data_ofs_(0)
    , buffer_size_(0)
    , vertex_count_(0)
    , resized_(false)
  {
  }

  ~VertexBuffer()
  {
    SAFE_FREE(double_buffer_);
  }

  void set_vertex_buffer()
  {
    ID3D10Buffer* bufs[] = { vb_ };
    uint32_t strides [] = { VertexSize };
    uint32_t offset = 0;
    device_->IASetVertexBuffers(0, 1, bufs, strides, &offset);
  }

  bool init(const D3D10_PASS_DESC& pass_desc)
  {
    buffer_size_ = 20000 * VertexSize;
    vb_.Attach(create_dynamic_buffer(device_, D3D10_BIND_VERTEX_BUFFER, buffer_size_));
    if (!vb_) {
      return false;
    }
    double_buffer_ = (uint8_t*)malloc(buffer_size_);

    layout_maker::InputDescs v;
    mpl::for_each<TList>(layout_maker(v));

    if (FAILED(device_->CreateInputLayout(&v[0], v.size(), 
      pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &input_layout_))) {
        return false;
    }

    return true;
  }

  void start_frame()
  {
    vertex_count_ = 0;
    data_ofs_ = 0;
  }

  bool end_frame()
  {
    if (resized_) {
      vb_->Release();

      vb_.Attach(create_dynamic_buffer(device_, D3D10_BIND_VERTEX_BUFFER, buffer_size_));
      if (!vb_) {
        return false;
      }
    }

    uint8_t* data = NULL;
    if (FAILED(vb_->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&data))) {
      LOG_WARNING_LN("Error mapping vertex buffer");
      return false;
    }
    memcpy(data, double_buffer_, vertex_count_ * VertexSize);
    vb_->Unmap();
    return true;
  }

  void add(const typename mpl::at<TList, mpl::int_<0> >::type& v)
  {
    const uint32_t elem_size = sizeof(v);
    check_for_resize(elem_size);

    memcpy(&double_buffer_[data_ofs_], &v, sizeof(v));
    data_ofs_ += sizeof(v);
    vertex_count_++;
  }

  void add(
    const typename mpl::at<TList, mpl::int_<0> >::type& v0,
    const typename mpl::at<TList, mpl::int_<1> >::type& v1)
  {
    const uint32_t elem_size = sizeof(v0) + sizeof(v1);
    check_for_resize(elem_size);

    *(typename mpl::at<TList, mpl::int_<0> >::type*)&double_buffer_[data_ofs_] = v0;
    *(typename mpl::at<TList, mpl::int_<1> >::type*)&double_buffer_[data_ofs_ + sizeof(v0)] = v1;
    data_ofs_ += elem_size;
    vertex_count_++;
  }

  void check_for_resize(const uint32_t elem_size)
  {
    if (data_ofs_ + elem_size > buffer_size_) {
      buffer_size_ *= 2;
      double_buffer_ = (uint8_t*)realloc(double_buffer_, buffer_size_);
      resized_ = true;
    }
  }
};

#endif // #ifndef VERTEX_BUFFER_HPP
