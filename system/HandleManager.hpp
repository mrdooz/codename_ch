/*
* Copyright (c) 2008, Power of Two Games LLC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Power of Two Games LLC nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY POWER OF TWO GAMES LLC ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL POWER OF TWO GAMES LLC BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef HANDLE_MANAGER_HPP
#define HANDLE_MANAGER_HPP

#include "Handle.hpp"
#include <boost/scoped_ptr.hpp>
#include <hash_map>

struct Technique;

typedef std::string TechniqueName;
typedef stdext::hash_map< TechniqueName, boost::shared_ptr<Technique> > NameToTechnique;

#ifdef STANDALONE
typedef void* PyObj;
#endif
struct Effect
{
  Effect(PyObj py_effect, const std::string& hlsl, ID3D10Effect* effect, const NameToTechnique& techniques) 
    : py_effect_(py_effect), hlsl_(hlsl), effect(effect), techniques(techniques) {}
  PyObj py_effect_;
  std::string hlsl_;
  ID3D10Effect* effect;
  NameToTechnique techniques;
};

struct Buffer
{
  Buffer(ID3D10Buffer* buffer, const uint32_t count, const uint32_t size) : buffer(buffer), count(count), size(size) {}

  ID3D10Buffer* buffer;
  uint32_t count;
  uint32_t size;
};

struct InputLayout
{
  InputLayout(ID3D10InputLayout* layout) : layout(layout) {}
  ID3D10InputLayout* layout;
};

class HandleManager
{
public:
	enum { MaxEntries = 4096 }; // 2^12

	HandleManager();
  ~HandleManager();

  void close();

  Handle add(Effect* p);
  Handle add(Buffer* p);
  Handle add(InputLayout* p);

  void update(const Handle& handle, Effect* p);
  void update(const Handle& handle, Buffer* p);
  void update(const Handle& handle, InputLayout* p);
	
  bool get(const Handle& handle, Effect*& p) const;
  bool get(const Handle& handle, Buffer*& p) const;
  bool get(const Handle& handle, InputLayout*& p) const;

  void remove(Handle handle);

  static Handle InvalidHandle;

private:
  void reset();	

  template<class T>
	struct HandleEntry
	{
    HandleEntry() : m_nextFreeIndex(0), counter_(1), m_active(0), m_endOfList(0), m_entry(NULL) {}
		explicit HandleEntry(uint32_t nextFreeIndex) : m_nextFreeIndex(nextFreeIndex), counter_(1), m_active(0), m_endOfList(0), m_entry(NULL) {}
		uint32_t m_nextFreeIndex : 12;
		uint32_t counter_ : 15;
		uint32_t m_active : 1;
		uint32_t m_endOfList : 1;
    T* m_entry;
	};

  template<class T>
  struct HandleData
  {
    ~HandleData()
    {
    }

    void close()
    {
      for (int32_t i = 0; i < MaxEntries; ++i) {
        SAFE_DELETE(data_[i].m_entry);
      }
    }

    void reset()
    {
      active_entry_count_ = 0;
      first_free_entry_ = 0;

      for (int i = 0; i < MaxEntries - 1; ++i) {
        data_[i] = HandleEntry<T>(i + 1);
      }
      data_[MaxEntries - 1] = HandleEntry<T>();
      data_[MaxEntries - 1].m_endOfList = true;
    }

    Handle add(T* p)
    {
      SUPER_ASSERT(active_entry_count_ < MaxEntries - 1);

      const int newIndex = first_free_entry_;
      SUPER_ASSERT(newIndex < MaxEntries);
      SUPER_ASSERT(data_[newIndex].m_active == false);
      SUPER_ASSERT(!data_[newIndex].m_endOfList);

      first_free_entry_ = data_[newIndex].m_nextFreeIndex;
      data_[newIndex].m_nextFreeIndex = 0;
      data_[newIndex].counter_ = data_[newIndex].counter_ + 1;
      if (data_[newIndex].counter_ == 0) {
        data_[newIndex].counter_ = 1;
      }
      data_[newIndex].m_active = true;
      data_[newIndex].m_entry = p;

      ++active_entry_count_;

      return Handle(newIndex, data_[newIndex].counter_, (HandleType::Enum)handle_type_from_type<T>::type);
    }

    void update(const Handle& handle, T* p)
    {	
      const int index = handle.index_;
      SUPER_ASSERT(data_[index].counter_ == handle.counter_);
      SUPER_ASSERT(data_[index].m_active == true);
      data_[index].m_entry = p;
    }

    void remove(const Handle& handle)
    {
      const uint32_t index = handle.index_;
      SUPER_ASSERT(data_[index].counter_ == handle.counter_);
      SUPER_ASSERT(data_[index].m_active == true);

      SAFE_DELETE(data_[index].m_entry);
      data_[index].m_nextFreeIndex = first_free_entry_;
      data_[index].m_active = 0;
      first_free_entry_ = index;

      --active_entry_count_;
    }

    bool get(const Handle handle, T*& out) const
    {
      const int index = handle.index_;
      if (data_[index].counter_ != handle.counter_ || data_[index].m_active == false) {
        return false;
      }

      out = data_[index].m_entry;
      return true;
    }

    int32_t active_entry_count_;
    uint32_t first_free_entry_;
    HandleEntry<T> data_[MaxEntries];
  };

  HandleData<Effect> effects_;
  HandleData<Buffer> buffers_;
  HandleData<InputLayout> input_layouts_;
};


#endif // HANDLE_MANAGER_HPP
