#pragma once

#include "common/base.h"

#include <vector>

namespace vx
{
template<typename T>
class array
{
  public:
    explicit array() {}
    explicit array(int size) : backend(size) {}
    VX_FORCE_INLINE T& add()
    {
        backend.push_back(T{});
        return backend[backend.size() - 1];
    }
    VX_FORCE_INLINE T& add(const T& t)
    {
        backend.push_back(t);
        return backend[backend.size() - 1];
    }
    VX_FORCE_INLINE void clear() { backend.clear(); }
    VX_FORCE_INLINE void resize(int size) { backend.resize(size); }
    VX_FORCE_INLINE void reserve(int size) { backend.reserve(size); }
    VX_FORCE_INLINE int size() const { return (int)backend.size(); }
    VX_FORCE_INLINE int byte_size() const { return (int)(sizeof(T) * size()); }
    VX_FORCE_INLINE T* ptr() { return &backend[0]; }
    VX_FORCE_INLINE const T* ptr() const { return &backend[0]; }
    VX_FORCE_INLINE T& operator[](int index) { return backend[index]; }
    VX_FORCE_INLINE const T& operator[](int index) const { return backend[index]; }

  private:
    std::vector<T> backend;
};
}