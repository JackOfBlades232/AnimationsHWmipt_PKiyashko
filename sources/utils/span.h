#pragma once
#include <cassert>

template <class T>
class Span
{
public:
  Span() = default;
  explicit Span(T *data, size_t size) : m_data(data), m_size(size) {}

  inline T &operator[](size_t id)
  {
    assert(m_data);
    assert(id < m_size);
    return m_data[id];
  }

  inline Span operator+(size_t offset)
  {
    assert(m_data);
    assert(offset < m_size);
    return Span(m_data + offset, m_size - offset);
  }
  
  inline const T *get() const { return m_data; }
  inline size_t size() const { return m_size; }

private:
  T *m_data{nullptr};
  size_t m_size{0};
};