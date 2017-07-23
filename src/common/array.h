#pragma once

#include <vector>

namespace vx
{
template<typename T>
class array
{
  public:
    explicit array() {}
    explicit array(int size) : backend(size) {}
    inline T& add()
    {
        backend.push_back(T{});
        return backend[backend.size() - 1];
    }
    inline T& add(const T& t)
    {
        backend.push_back(t);
        return backend[backend.size() - 1];
    }
    inline void clear() { backend.clear(); }
    inline void resize(int size) { backend.resize(size); }
    inline void reserve(int size) { backend.reserve(size); }
    inline int size() const { return (int)backend.size(); }
    inline int byte_size() const { return (int)(sizeof(T) * size()); }
    inline T* ptr() { return &backend[0]; }
    inline const T* ptr() const { return &backend[0]; }
    inline T& operator[](int index) { return backend[index]; }
    inline const T& operator[](int index) const { return backend[index]; }

  private:
    std::vector<T> backend;
};
}