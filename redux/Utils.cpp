#include "stdafx.h"
#include "Utils.hpp"

ID3D10Buffer* create_dynamic_buffer(ID3D10DevicePtr device, const uint32_t bind_flag, const uint32_t buffer_size)
{
  D3D10_BUFFER_DESC buffer_desc;
  ZeroMemory(&buffer_desc, sizeof(buffer_desc));
  buffer_desc.Usage     = D3D10_USAGE_DYNAMIC;
  buffer_desc.ByteWidth = buffer_size;
  buffer_desc.BindFlags = bind_flag;
  buffer_desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

  ID3D10Buffer* buffer = NULL;
  if (FAILED(device->CreateBuffer(&buffer_desc, NULL, &buffer))) {
    return NULL;
  }
  return buffer;
}
