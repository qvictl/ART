#ifndef ART_UTIL_MMAP_H_
#define ART_UTIL_MMAP_H_

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace art {

class MemoryMappedFile {
public:
  MemoryMappedFile(const std::string &file_path) : data_(nullptr), size_(0) {
#ifdef _WIN32
    file_handle_ =
        CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle_ == INVALID_HANDLE_VALUE) {
      throw std::runtime_error("Failed to open file");
    }

    file_mapping_ =
        CreateFileMapping(file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!file_mapping_) {
      CloseHandle(file_handle_);
      throw std::runtime_error("Failed to create file mapping");
    }

    data_ = MapViewOfFile(file_mapping_, FILE_MAP_READ, 0, 0, 0);
    if (!data_) {
      CloseHandle(file_mapping_);
      CloseHandle(file_handle_);
      throw std::runtime_error("Failed to map view of file");
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
      UnmapViewOfFile(data_);
      CloseHandle(file_mapping_);
      CloseHandle(file_handle_);
      throw std::runtime_error("Failed to get file size");
    }
    size_ = static_cast<size_t>(file_size.QuadPart);
#else
    file_descriptor_ = open(file_path.c_str(), O_RDONLY);
    if (file_descriptor_ == -1) {
      throw std::runtime_error("Failed to open file");
    }

    struct stat fileStat;
    if (fstat(file_descriptor_, &fileStat) == -1) {
      close(file_descriptor_);
      throw std::runtime_error("Failed to get file size");
    }
    size_ = static_cast<size_t>(fileStat.st_size);

    data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, file_descriptor_, 0);
    if (data_ == MAP_FAILED) {
      close(file_descriptor_);
      throw std::runtime_error("Failed to map file");
    }
#endif
  }

  ~MemoryMappedFile() {
#ifdef _WIN32
    if (data_) {
      UnmapViewOfFile(data_);
    }
    if (file_mapping_) {
      CloseHandle(file_mapping_);
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(file_handle_);
    }
#else
    if (data_) {
      munmap(data_, size_);
    }
    if (file_descriptor_ != -1) {
      close(file_descriptor_);
    }
#endif
  }

  const void *data() const { return data_; }
  size_t size() const { return size_; }

private:
  void *data_;
  size_t size_;

#ifdef _WIN32
  HANDLE file_handle_;
  HANDLE file_mapping_;
#else
  int file_descriptor_;
#endif
};

} // namespace art

#endif // ART_UTIL_MMAP_H_
