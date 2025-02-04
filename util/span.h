#ifndef ART_UTIL_SPAN_H_
#define ART_UTIL_SPAN_H_

#include <array>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace art {
// Simple implementation of std::span since C++20, while not completely
// compatible. The most significant difference is that subspan() can take
// nagative offset, much like Python's list slicing.
template <typename T> class Span {
public:
  // Constructors
  constexpr Span() noexcept : data_(nullptr), size_(0) {}

  // Constructor for const arrays or containers
  constexpr Span(const T *ptr, std::size_t size) noexcept
      : data_(ptr), size_(size) {}

  // Constructor for const C-style arrays
  template <std::size_t N>
  constexpr Span(const T (&arr)[N]) noexcept : data_(arr), size_(N) {}

  // Constructor for const containers (e.g., std::vector, std::array)
  template <typename Container,
            typename = std::enable_if_t<
                !std::is_array<Container>::value &&
                std::is_convertible_v<
                    decltype(std::declval<Container>().data()), const T *>>>
  constexpr Span(const Container &container) noexcept
      : data_(container.data()), size_(container.size()) {}

  // Accessors
  constexpr const T *data() const noexcept { return data_; }
  constexpr std::size_t size() const noexcept { return size_; }
  constexpr bool empty() const noexcept { return size_ == 0; }

  // Element access
  constexpr const T &operator[](std::size_t idx) const {
    if (idx >= size_) {
      throw std::out_of_range("Span index out of range");
    }
    return data_[idx];
  }

  constexpr const T &front() const {
    if (empty()) {
      throw std::out_of_range("Span is empty");
    }
    return data_[0];
  }

  constexpr const T &back() const {
    if (empty()) {
      throw std::out_of_range("Span is empty");
    }
    return data_[size_ - 1];
  }

  // Subviews
  constexpr Span subspan(std::size_t offset, std::size_t count) const {
    if (offset > size_ || count > size_ - offset) {
      throw std::out_of_range("Invalid subspan range");
    }
    return Span(data_ + offset, count);
  }

  constexpr Span subspan(int offset) const {
    if (std::abs(offset) > size_) {
      throw std::out_of_range("Invalid subspan offset");
    }
    if (offset < 0) {
      return Span(data_, size_ + offset);
    } else {
      return Span(data_ + offset, size_ - offset);
    }
  }

  constexpr Span first(std::size_t count) const {
    if (count > size_) {
      throw std::out_of_range("Invalid count for first");
    }
    return Span(data_, count);
  }

  constexpr Span last(std::size_t count) const {
    if (count > size_) {
      throw std::out_of_range("Invalid count for last");
    }
    return Span(data_ + size_ - count, count);
  }

  // Iterators
  constexpr const T *begin() const noexcept { return data_; }
  constexpr const T *end() const noexcept { return data_ + size_; }

private:
  const T *data_;
  std::size_t size_;
};

template <typename Container>
constexpr Span<typename Container::value_type>
MakeSpan(const Container &container) {
  return Span<typename Container::value_type>(container);
}

} // namespace art

#endif // ART_UTIL_SPAN_H_
