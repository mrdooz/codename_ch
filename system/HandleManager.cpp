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

#include "stdafx.h"
#include "HandleManager.hpp"

Handle HandleManager::InvalidHandle = Handle();

HandleManager::HandleManager()
{
	reset();
}

HandleManager::~HandleManager()
{
  close();
}

void HandleManager::close()
{
  effects_.close();
  buffers_.close();
  input_layouts_.close();
}

void HandleManager::reset()
{
  effects_.reset();
  buffers_.reset();
  input_layouts_.reset();
}

Handle HandleManager::add(Effect* p)
{
  return effects_.add(p);
}

Handle HandleManager::add(Buffer* p)
{
  return buffers_.add(p);
}

Handle HandleManager::add(InputLayout* p)
{
  return input_layouts_.add(p);
}

void HandleManager::update(const Handle& handle, Effect* p)
{
  effects_.update(handle, p);
}

void HandleManager::update(const Handle& handle, Buffer* p)
{
  buffers_.update(handle, p);
}

void HandleManager::update(const Handle& handle, InputLayout* p)
{
  input_layouts_.update(handle, p);
}


void HandleManager::remove(const Handle handle)
{
  switch( handle.type_ ) {
    case HandleType::Effect:
      return effects_.remove(handle);
    case HandleType::Buffer:
      return effects_.remove(handle);
    case HandleType::InputLayout:
      return effects_.remove(handle);
  }
  SUPER_ASSERT(false);
}

bool HandleManager::get(const Handle& handle, Effect*& p) const
{
  return effects_.get(handle, p);
}

bool HandleManager::get(const Handle& handle, Buffer*& p) const
{
  return buffers_.get(handle, p);
}

bool HandleManager::get(const Handle& handle, InputLayout*& p) const
{
  return input_layouts_.get(handle, p);
}

