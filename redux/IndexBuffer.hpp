#ifndef INDEX_BUFFER_HPP
#define INDEX_BUFFER_HPP

#include "Utils.hpp"

template<typename IndexType>
struct IndexBuffer
{
  ID3D10BufferPtr ib_;
  ID3D10DevicePtr device_;
  IndexType* double_buffer_;
  uint32_t index_count_;
  uint32_t buffer_size_;
  bool resized_;


  IndexBuffer(const ID3D10DevicePtr& device)
    : device_(device)
    , double_buffer_(NULL)
    , index_count_(0)
    , buffer_size_(0)
    , resized_(false)
  {
  }

  ~IndexBuffer()
  {
    SAFE_FREE(double_buffer_);
  }

  uint32_t index_count() const { return index_count_; }

  void set_index_buffer()
  {
    device_->IASetIndexBuffer(ib_, DXGI_FORMAT_R32_UINT, 0);
  }

  bool init()
  {
    buffer_size_ = 1000 * sizeof(IndexType);
    double_buffer_ = (IndexType*)malloc(buffer_size_);
    ib_.Attach(create_dynamic_buffer(device_, D3D10_BIND_INDEX_BUFFER, buffer_size_));
    if (!ib_) {
      return false;
    }
    return true;
  }

  void start_frame()
  {
    index_count_ = 0;
  }


  bool end_frame()
  {
    if (resized_) {
      ib_.Release();
      ID3D10Buffer* buf = create_dynamic_buffer(device_, D3D10_BIND_INDEX_BUFFER, buffer_size_);
      ib_.Attach(buf);
      resized_ = false;
    }

    uint8_t* data = NULL;
    if (FAILED(ib_->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&data))) {
      LOG_WARNING_LN("Error mapping index buffer");
      return false;
    }
    memcpy(data, double_buffer_, index_count_ * sizeof(IndexType));
    ib_->Unmap();
    return true;
  }

  void add(const IndexType idx)
  {
    if ((index_count_ + 1) * sizeof(IndexType) > buffer_size_) {
      buffer_size_ *= 2;
      double_buffer_ = (IndexType*)realloc(double_buffer_, buffer_size_);
      resized_ = true;
    }
    double_buffer_[index_count_++] = idx;
  }

};


#endif // #ifndef INDEX_BUFFER_HPP
