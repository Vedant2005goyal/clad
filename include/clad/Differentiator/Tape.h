// #ifndef CLAD_TAPE_H
// #define CLAD_TAPE_H

// #include "clad/Differentiator/CladConfig.h"
// #include <cassert>
// #include <cstddef>
// #include <cstdint>
// #include <cstdio>
// #include <fstream>
// #include <ios>
// #include <iterator>
// #include <memory>
// #include <new>
// #include <string>
// #include <type_traits>
// #include <utility>
// #ifndef __CUDACC__
// #include <mutex>
// #endif

// namespace clad {

// namespace detail {

// template <typename T, std::size_t SLAB_SIZE> struct DiskManager {
// #ifndef __CUDA_ARCH__
//   std::fstream file;
//   std::string filename;
//   DiskManager() {
//     filename = "clad_tape_" + std::to_string((uintptr_t)this) + ".tmp";
//     file.open(filename, std::ios::in | std::ios::out | std::ios::binary |
//                             std::ios::trunc);
//   }
//   ~DiskManager() {
//     if (file.is_open())
//       file.close();
//     std::remove(filename.c_str());
//   }
//   std::size_t write_slab(const T* data) {
//     file.seekp(0, std::ios::end);
//     std::size_t offset = file.tellp();
//     const void* raw_data = static_cast<const void*>(data);
//     file.write(static_cast<const char*>(raw_data), SLAB_SIZE * sizeof(T));
//     return offset;
//   }
//   void read_slab(T* dest, std::size_t offset) {
//     file.seekg(offset, std::ios::beg);
//     void* raw_dest = static_cast<void*>(dest);
//     file.read(static_cast<char*>(raw_dest), SLAB_SIZE * sizeof(T));
//   }
// #else
//   CUDA_HOST_DEVICE DiskManager() {}
//   CUDA_HOST_DEVICE ~DiskManager() {}
//   CUDA_HOST_DEVICE std::size_t write_slab(const T* data) { return 0; }
//   CUDA_HOST_DEVICE void read_slab(T* dest, std::size_t offset) {}
// #endif
// };

// template <bool Enabled, typename T, std::size_t SLAB_SIZE>
// struct OffloadStorage;

// template <typename T, std::size_t SLAB_SIZE>
// struct OffloadStorage<false, T, SLAB_SIZE> {};

// template <typename T, std::size_t SLAB_SIZE>
// struct OffloadStorage<true, T, SLAB_SIZE> {
//   std::unique_ptr<DiskManager<T, SLAB_SIZE>> m_DiskManager;
//   std::size_t m_ActiveSlabs = 0;
//   std::size_t m_MaxRamSlabs = 1000;
// };

// } // namespace detail

// template <typename T, std::size_t SBO_SIZE, std::size_t SLAB_SIZE,
//           bool is_multithread, bool DiskOffload>
// class tape_impl;

// template <typename T, std::size_t SBO_SIZE = 64, std::size_t SLAB_SIZE = 1024,
//           bool is_multithread = false, bool DiskOffload = false>
// class tape_iterator {
//   using tape_t =
//       clad::tape_impl<T, SBO_SIZE, SLAB_SIZE, is_multithread, DiskOffload>;
//   tape_t* m_tape;
//   std::size_t m_index;

// public:
//   using iterator_category = std::forward_iterator_tag;
//   using value_type = T;
//   using difference_type = std::ptrdiff_t;
//   using pointer = T*;
//   using reference = T&;

//   CUDA_HOST_DEVICE tape_iterator() : m_tape(nullptr), m_index(0) {}
//   CUDA_HOST_DEVICE tape_iterator(tape_t* tape, std::size_t index)
//       : m_tape(tape), m_index(index) {}

//   CUDA_HOST_DEVICE reference operator*() const { return (*m_tape)[m_index]; }
//   CUDA_HOST_DEVICE pointer operator->() const { return &(*m_tape)[m_index]; }

//   CUDA_HOST_DEVICE tape_iterator& operator++() {
//     ++m_index;
//     return *this;
//   }

//   CUDA_HOST_DEVICE tape_iterator operator++(int) {
//     tape_iterator tmp = *this;
//     ++(*this);
//     return tmp;
//   }

//   CUDA_HOST_DEVICE bool operator==(const tape_iterator& other) const {
//     return m_index == other.m_index;
//   }

//   CUDA_HOST_DEVICE bool operator!=(const tape_iterator& other) const {
//     return m_index != other.m_index;
//   }
// };

// template <typename T, std::size_t SBO_SIZE = 64, std::size_t SLAB_SIZE = 1024,
//           bool is_multithread = false, bool DiskOffload = false>
// class tape_impl {
//   struct RAMStorage {
//     alignas(T) char raw_data[SLAB_SIZE * sizeof(T)];
//     CUDA_HOST_DEVICE RAMStorage() {}
//     CUDA_HOST_DEVICE T* elements() {
// #if __cplusplus >= 201703L
//       return std::launder(reinterpret_cast<T*>(raw_data));
// #else
//       return reinterpret_cast<T*>(raw_data);
// #endif
//     }
//   };

//   struct DiskStorage {
//     T* data_ptr = nullptr;
//     bool is_on_disk = false;
//     std::size_t disk_offset = 0;

//     CUDA_HOST_DEVICE DiskStorage() { allocate(); }
//     CUDA_HOST_DEVICE ~DiskStorage() { deallocate(); }

//     DiskStorage(const DiskStorage&) = delete;
//     DiskStorage& operator=(const DiskStorage&) = delete;

//     void allocate() {
//       if (!data_ptr)
//         data_ptr = static_cast<T*>(::operator new(SLAB_SIZE * sizeof(T)));
//     }

//     void deallocate() {
//       if (data_ptr) {
//         ::operator delete(data_ptr);
//         data_ptr = nullptr;
//       }
//     }

//     CUDA_HOST_DEVICE T* elements() { return data_ptr; }
//   };

//   using SlabBase = std::conditional_t<DiskOffload, DiskStorage, RAMStorage>;

// public:
//   struct Slab : public SlabBase {
//     Slab* prev;
//     Slab* next;

//     CUDA_HOST_DEVICE Slab() : prev(nullptr), next(nullptr) {}
//   };

// public:
//   alignas(T) char m_static_buffer[SBO_SIZE * sizeof(T)];

//   Slab* m_head = nullptr;
//   Slab* m_tail = nullptr;
//   std::size_t m_size = 0;
//   std::size_t m_capacity = SBO_SIZE;

// #ifndef __CUDACC__
//   mutable std::mutex m_TapeMutex;
// #endif

//   detail::OffloadStorage<DiskOffload, T, SLAB_SIZE> m_Offload;

//   CUDA_HOST_DEVICE T* sbo_elements() {
// #if __cplusplus >= 201703L
//     return std::launder(reinterpret_cast<T*>(m_static_buffer));
// #else
//     return reinterpret_cast<T*>(m_static_buffer);
// #endif
//   }

//   CUDA_HOST_DEVICE const T* sbo_elements() const {
// #if __cplusplus >= 201703L
//     return std::launder(reinterpret_cast<const T*>(m_static_buffer));
// #else
//     return reinterpret_cast<const T*>(m_static_buffer);
// #endif
//   }

//   inline __attribute__((always_inline)) void check_and_evict(std::true_type) {
//     if (m_Offload.m_ActiveSlabs >= m_Offload.m_MaxRamSlabs) {
//       Slab* candidate = m_head;
//       while (candidate && (candidate->is_on_disk || candidate == m_tail))
//         candidate = candidate->next;
//       if (candidate) {
//         if (!m_Offload.m_DiskManager)
//           m_Offload.m_DiskManager.reset(
//               new detail::DiskManager<T, SLAB_SIZE>());
//         candidate->disk_offset =
//             m_Offload.m_DiskManager->write_slab(candidate->elements());
//         candidate->deallocate();
//         candidate->is_on_disk = true;
//         m_Offload.m_ActiveSlabs--;
//       }
//     }
//   }
//   inline __attribute__((always_inline)) void check_and_evict(std::false_type) {}

//   inline __attribute__((always_inline)) void ensure_loaded(Slab* slab,
//                                                            std::true_type) {
//     if (slab && slab->is_on_disk) {
//       if (m_Offload.m_ActiveSlabs >= m_Offload.m_MaxRamSlabs) {
//         Slab* v = m_head;
//         while (v) {
//           if (!v->is_on_disk && v != slab) {
//             v->disk_offset = m_Offload.m_DiskManager->write_slab(v->elements());
//             v->deallocate();
//             v->is_on_disk = true;
//             m_Offload.m_ActiveSlabs--;
//             break;
//           }
//           v = v->next;
//         }
//       }
//       slab->allocate();
//       m_Offload.m_DiskManager->read_slab(slab->elements(), slab->disk_offset);
//       slab->is_on_disk = false;
//       m_Offload.m_ActiveSlabs++;
//     }
//   }
//   inline __attribute__((always_inline)) void ensure_loaded(Slab* slab,
//                                                            std::false_type) {}

//   inline __attribute__((always_inline)) void track_new_slab(std::true_type) {
//     m_Offload.m_ActiveSlabs++;
//   }
//   inline __attribute__((always_inline)) void track_new_slab(std::false_type) {}

// public:
//   using reference = T&;
//   using const_reference = const T&;
//   using pointer = T*;
//   using const_pointer = const T*;
//   using size_type = std::size_t;
//   using difference_type = std::ptrdiff_t;
//   using value_type = T;
//   using iterator =
//       tape_iterator<T, SBO_SIZE, SLAB_SIZE, is_multithread, DiskOffload>;
//   using const_iterator =
//       tape_iterator<const T, SBO_SIZE, SLAB_SIZE, is_multithread, DiskOffload>;

// #ifndef __CUDACC__
//   std::mutex& mutex() const { return m_TapeMutex; }
// #endif

//   CUDA_HOST_DEVICE tape_impl() = default;

//   tape_impl(const tape_impl&) = delete;
//   tape_impl& operator=(const tape_impl&) = delete;
//   tape_impl(tape_impl&&) = delete;
//   tape_impl& operator=(tape_impl&&) = delete;

//   CUDA_HOST_DEVICE ~tape_impl() { clear(); }

//   template <typename... ArgsT>
//   CUDA_HOST_DEVICE void emplace_back(ArgsT&&... args) {
//     if (m_size < SBO_SIZE) {
//       ::new (const_cast<void*>(static_cast<const volatile void*>(
//           sbo_elements() + m_size))) T(std::forward<ArgsT>(args)...);
//     } else {
//       const auto offset = (m_size - SBO_SIZE) % SLAB_SIZE;

//       if (!offset) {
//         if (m_size == m_capacity) {
//           check_and_evict(std::integral_constant<bool, DiskOffload>{});

//           Slab* new_slab = new Slab();
//           track_new_slab(std::integral_constant<bool, DiskOffload>{});
//           if (!m_head)
//             m_head = new_slab;
//           else {
//             m_tail->next = new_slab;
//             new_slab->prev = m_tail;
//           }
//           m_capacity += SLAB_SIZE;
//         }
//         if (m_size == SBO_SIZE)
//           m_tail = m_head;
//         else
//           m_tail = m_tail->next;
//       }

//       ensure_loaded(m_tail, std::integral_constant<bool, DiskOffload>{});
//       ::new (const_cast<void*>(static_cast<const volatile void*>(
//           m_tail->elements() + offset))) T(std::forward<ArgsT>(args)...);
//     }
//     m_size++;
//   }

//   CUDA_HOST_DEVICE std::size_t size() const { return m_size; }
//   CUDA_HOST_DEVICE iterator begin() { return iterator(this, 0); }
//   CUDA_HOST_DEVICE const_iterator begin() const {
//     return const_iterator(this, 0);
//   }
//   CUDA_HOST_DEVICE iterator end() { return iterator(this, m_size); }
//   CUDA_HOST_DEVICE const_iterator end() const {
//     return const_iterator(this, m_size);
//   }

//   CUDA_HOST_DEVICE reference back() {
//     assert(m_size);
//     return (*this)[m_size - 1];
//   }

//   CUDA_HOST_DEVICE const_reference back() const {
//     assert(m_size);
//     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
//     return const_cast<tape_impl*>(this)->operator[](m_size - 1);
//   }

//   CUDA_HOST_DEVICE reference operator[](std::size_t i) {
//     assert(i < m_size);
//     return *at(i);
//   }

//   CUDA_HOST_DEVICE const_reference operator[](std::size_t i) const {
//     assert(i < m_size);
//     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
//     return *const_cast<tape_impl*>(this)->at(i);
//   }

//   CUDA_HOST_DEVICE void pop_back() {
//     assert(m_size);
//     m_size--;
//     if (m_size < SBO_SIZE)
//       destroy_element(sbo_elements() + m_size);
//     else {
//       ensure_loaded(m_tail, std::integral_constant<bool, DiskOffload>{});

//       std::size_t offset = (m_size - SBO_SIZE) % SLAB_SIZE;
//       destroy_element(m_tail->elements() + offset);
//       if (offset == 0) {
//         if (m_tail != m_head)
//           m_tail = m_tail->prev;
//       }
//     }
//   }

// public:
//   /// Returns pointer to element at specified index, handling SBO or slab lookup
//   CUDA_HOST_DEVICE T* at(std::size_t index) {
//     if (index < SBO_SIZE)
//       return sbo_elements() + index;

//     Slab* slab = m_head;
//     std::size_t idx = (index - SBO_SIZE) / SLAB_SIZE;
//     while (idx--)
//       slab = slab->next;

//     ensure_loaded(slab, std::integral_constant<bool, DiskOffload>{});

//     return slab->elements() + ((index - SBO_SIZE) % SLAB_SIZE);
//   }

//   template <typename It> using value_type_of = decltype(*std::declval<It>());

//   // Call destructor for every value in the given range.
//   template <typename It>
//   static typename std::enable_if<
//       !std::is_trivially_destructible<value_type_of<It>>::value>::type
//   destroy(It B, It E) {
//     for (It I = E - 1; I >= B; --I)
//       I->~value_type_of<It>();
//   }

//   // If type is trivially destructible, its destructor is no-op, so we can avoid
//   // for loop here.
//   template <typename It>
//   static typename std::enable_if<
//       std::is_trivially_destructible<value_type_of<It>>::value>::type
//       CUDA_HOST_DEVICE
//       destroy(It B, It E) {}

//   /// Destroys all elements and deallocates slabs
//   void clear() {
//     std::size_t count = m_size;
//     for (std::size_t i = 0; i < SBO_SIZE && count > 0; ++i, --count)
//       destroy_element(&sbo_elements()[i]);

//     Slab* slab = m_head;
//     while (slab) {
//       if constexpr (!DiskOffload) {
//         T* elems = slab->elements();
//         for (size_t i = 0; i < SLAB_SIZE && count > 0; ++i, --count)
//           destroy_element(elems + i);
//       } else {
//         size_t current_slab_count = (count > SLAB_SIZE) ? SLAB_SIZE : count;
//         count -= current_slab_count;
//       }

//       Slab* tmp = slab;
//       slab = slab->next;
//       delete tmp;
//     }

//     m_head = nullptr;
//     m_tail = nullptr;
//     m_size = 0;
//     m_capacity = SBO_SIZE;
//     if constexpr (DiskOffload)
//       m_Offload.m_ActiveSlabs = 0;
//   }

//   template <typename ElTy> void destroy_element(ElTy* elem) { elem->~ElTy(); }
//   template <typename ElTy, size_t N> void destroy_element(ElTy (*arr)[N]) {
//     for (size_t i = 0; i < N; ++i)
//       (*arr)[i].~ElTy();
//   }
// };
// } // namespace clad

// #endif // CLAD_TAPE_H
#ifndef CLAD_TAPE_H
#define CLAD_TAPE_H

#include "clad/Differentiator/CladConfig.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#ifndef __CUDACC__
#include <mutex>
#endif

namespace clad {

namespace detail {
// --- Helper: DiskManager (Only used in the Specialization) ---
template <typename T, std::size_t SLAB_SIZE> struct DiskManager {
#ifndef __CUDA_ARCH__
  std::fstream file;
  std::string filename;
  DiskManager() {
    filename = "clad_tape_" + std::to_string((uintptr_t)this) + ".tmp";
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary |
                            std::ios::trunc);
  }
  ~DiskManager() {
    if (file.is_open())
      file.close();
    std::remove(filename.c_str());
  }
  std::size_t write_slab(const T* data) {
    file.seekp(0, std::ios::end);
    std::size_t offset = file.tellp();
    const void* raw_data = static_cast<const void*>(data);
    file.write(static_cast<const char*>(raw_data), SLAB_SIZE * sizeof(T));
    return offset;
  }
  void read_slab(T* dest, std::size_t offset) {
    file.seekg(offset, std::ios::beg);
    void* raw_dest = static_cast<void*>(dest);
    file.read(static_cast<char*>(raw_dest), SLAB_SIZE * sizeof(T));
  }
#else
  CUDA_HOST_DEVICE DiskManager() {}
  CUDA_HOST_DEVICE ~DiskManager() {}
  CUDA_HOST_DEVICE std::size_t write_slab(const T* data) { return 0; }
  CUDA_HOST_DEVICE void read_slab(T* dest, std::size_t offset) {}
#endif
};
} // namespace detail

template <typename T, std::size_t SBO_SIZE, std::size_t SLAB_SIZE,
          bool is_multithread, bool DiskOffload>
class tape_impl;

template <typename T, std::size_t SBO_SIZE = 64, std::size_t SLAB_SIZE = 1024,
          bool is_multithread = false, bool DiskOffload = false>
class tape_iterator {
  using tape_t =
      clad::tape_impl<T, SBO_SIZE, SLAB_SIZE, is_multithread, DiskOffload>;
  tape_t* m_tape;
  std::size_t m_index;

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  CUDA_HOST_DEVICE tape_iterator() : m_tape(nullptr), m_index(0) {}
  CUDA_HOST_DEVICE tape_iterator(tape_t* tape, std::size_t index)
      : m_tape(tape), m_index(index) {}

  CUDA_HOST_DEVICE reference operator*() const { return (*m_tape)[m_index]; }
  CUDA_HOST_DEVICE pointer operator->() const { return &(*m_tape)[m_index]; }

  CUDA_HOST_DEVICE tape_iterator& operator++() {
    ++m_index;
    return *this;
  }

  CUDA_HOST_DEVICE tape_iterator operator++(int) {
    tape_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  CUDA_HOST_DEVICE bool operator==(const tape_iterator& other) const {
    return m_index == other.m_index;
  }

  CUDA_HOST_DEVICE bool operator!=(const tape_iterator& other) const {
    return m_index != other.m_index;
  }
};

// =============================================================================
// 1. PRIMARY TEMPLATE (DiskOffload = false)
// =============================================================================
// This is the EXACT ORIGINAL IMPLEMENTATION. Zero Overhead Guaranteed.
template <typename T, std::size_t SBO_SIZE = 64, std::size_t SLAB_SIZE = 1024,
          bool is_multithread = false, bool DiskOffload = false>
class tape_impl {
  struct Slab {
    alignas(T) char raw_data[SLAB_SIZE * sizeof(T)];
    Slab* prev;
    Slab* next;
    CUDA_HOST_DEVICE Slab() : prev(nullptr), next(nullptr) {}
    CUDA_HOST_DEVICE T* elements() {
#if __cplusplus >= 201703L
      return std::launder(reinterpret_cast<T*>(raw_data));
#else
      return reinterpret_cast<T*>(raw_data);
#endif
    }
  };

  alignas(T) char m_static_buffer[SBO_SIZE * sizeof(T)];
  Slab* m_head = nullptr;
  Slab* m_tail = nullptr;
  std::size_t m_size = 0;
  std::size_t m_capacity = SBO_SIZE;

#ifndef __CUDACC__
  mutable std::mutex m_TapeMutex;
#endif

  CUDA_HOST_DEVICE T* sbo_elements() {
#if __cplusplus >= 201703L
    return std::launder(reinterpret_cast<T*>(m_static_buffer));
#else
    return reinterpret_cast<T*>(m_static_buffer);
#endif
  }

  CUDA_HOST_DEVICE const T* sbo_elements() const {
#if __cplusplus >= 201703L
    return std::launder(reinterpret_cast<const T*>(m_static_buffer));
#else
    return reinterpret_cast<const T*>(m_static_buffer);
#endif
  }

public:
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using iterator = tape_iterator<T, SBO_SIZE, SLAB_SIZE, is_multithread, false>;
  using const_iterator =
      tape_iterator<const T, SBO_SIZE, SLAB_SIZE, is_multithread, false>;

#ifndef __CUDACC__
  std::mutex& mutex() const { return m_TapeMutex; }
#endif

  CUDA_HOST_DEVICE tape_impl() = default;
  CUDA_HOST_DEVICE ~tape_impl() { clear(); }

  template <typename... ArgsT>
  CUDA_HOST_DEVICE void emplace_back(ArgsT&&... args) {
    if (m_size < SBO_SIZE) {
      ::new (const_cast<void*>(static_cast<const volatile void*>(
          sbo_elements() + m_size))) T(std::forward<ArgsT>(args)...);
    } else {
      const auto offset = (m_size - SBO_SIZE) % SLAB_SIZE;
      if (!offset) {
        if (m_size == m_capacity) {
          Slab* new_slab = new Slab();
          if (!m_head)
            m_head = new_slab;
          else {
            m_tail->next = new_slab;
            new_slab->prev = m_tail;
          }
          m_capacity += SLAB_SIZE;
        }
        if (m_size == SBO_SIZE)
          m_tail = m_head;
        else
          m_tail = m_tail->next;
      }
      ::new (const_cast<void*>(static_cast<const volatile void*>(
          m_tail->elements() + offset))) T(std::forward<ArgsT>(args)...);
    }
    m_size++;
  }

  CUDA_HOST_DEVICE std::size_t size() const { return m_size; }
  CUDA_HOST_DEVICE iterator begin() { return iterator(this, 0); }
  CUDA_HOST_DEVICE const_iterator begin() const {
    return const_iterator(this, 0);
  }
  CUDA_HOST_DEVICE iterator end() { return iterator(this, m_size); }
  CUDA_HOST_DEVICE const_iterator end() const {
    return const_iterator(this, m_size);
  }

  CUDA_HOST_DEVICE reference back() {
    assert(m_size);
    return (*this)[m_size - 1];
  }

  CUDA_HOST_DEVICE const_reference back() const {
    assert(m_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<tape_impl*>(this)->operator[](m_size - 1);
  }

  CUDA_HOST_DEVICE reference operator[](std::size_t i) {
    assert(i < m_size);
    return *at(i);
  }

  CUDA_HOST_DEVICE const_reference operator[](std::size_t i) const {
    assert(i < m_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return *const_cast<tape_impl*>(this)->at(i);
  }

  CUDA_HOST_DEVICE void pop_back() {
    assert(m_size);
    m_size--;
    if (m_size < SBO_SIZE)
      destroy_element(sbo_elements() + m_size);
    else {
      std::size_t offset = (m_size - SBO_SIZE) % SLAB_SIZE;
      destroy_element(m_tail->elements() + offset);
      if (offset == 0) {
        if (m_tail != m_head)
          m_tail = m_tail->prev;
      }
    }
  }

private:
  CUDA_HOST_DEVICE T* at(std::size_t index) {
    if (index < SBO_SIZE)
      return sbo_elements() + index;
    Slab* slab = m_head;
    std::size_t idx = (index - SBO_SIZE) / SLAB_SIZE;
    while (idx--)
      slab = slab->next;
    return slab->elements() + ((index - SBO_SIZE) % SLAB_SIZE);
  }

  template <typename It> using value_type_of = decltype(*std::declval<It>());
  template <typename It>
  static typename std::enable_if<
      !std::is_trivially_destructible<value_type_of<It>>::value>::type
  destroy(It B, It E) {
    for (It I = E - 1; I >= B; --I)
      I->~value_type_of<It>();
  }
  template <typename It>
  static typename std::enable_if<
      std::is_trivially_destructible<value_type_of<It>>::value>::type
      CUDA_HOST_DEVICE
      destroy(It B, It E) {}

  void clear() {
    std::size_t count = m_size;
    for (std::size_t i = 0; i < SBO_SIZE && count > 0; ++i, --count)
      destroy_element(&sbo_elements()[i]);
    Slab* slab = m_head;
    while (slab) {
      T* elems = slab->elements();
      for (size_t i = 0; i < SLAB_SIZE && count > 0; ++i, --count)
        destroy_element(elems + i);
      Slab* tmp = slab;
      slab = slab->next;
      delete tmp;
    }
    m_head = nullptr;
    m_tail = nullptr;
    m_size = 0;
    m_capacity = SBO_SIZE;
  }

  template <typename ElTy> void destroy_element(ElTy* elem) { elem->~ElTy(); }
  template <typename ElTy, size_t N> void destroy_element(ElTy (*arr)[N]) {
    for (size_t i = 0; i < N; ++i)
      (*arr)[i].~ElTy();
  }
};

// =============================================================================
// 2. SPECIALIZATION (DiskOffload = true)
// =============================================================================
// This contains the NEW features. It is allowed to be larger/different.
template <typename T, std::size_t SBO_SIZE, std::size_t SLAB_SIZE,
          bool is_multithread>
class tape_impl<T, SBO_SIZE, SLAB_SIZE, is_multithread, true> {
  struct Slab {
    T* data_ptr = nullptr;
    bool is_on_disk = false;
    std::size_t disk_offset = 0;
    Slab* prev;
    Slab* next;

    CUDA_HOST_DEVICE Slab() : prev(nullptr), next(nullptr) { allocate(); }
    CUDA_HOST_DEVICE ~Slab() { deallocate(); }
    
    // Rule of 5: Delete copy/move to prevent double-free issues
    Slab(const Slab&) = delete;
    Slab& operator=(const Slab&) = delete;

    void allocate() {
      if (!data_ptr)
        data_ptr = static_cast<T*>(::operator new(SLAB_SIZE * sizeof(T)));
    }
    void deallocate() {
      if (data_ptr) {
        ::operator delete(data_ptr);
        data_ptr = nullptr;
      }
    }
    CUDA_HOST_DEVICE T* elements() { return data_ptr; }
  };

  alignas(T) char m_static_buffer[SBO_SIZE * sizeof(T)];
  Slab* m_head = nullptr;
  Slab* m_tail = nullptr;
  std::size_t m_size = 0;
  std::size_t m_capacity = SBO_SIZE;

#ifndef __CUDACC__
  mutable std::mutex m_TapeMutex;
#endif

  // --- Disk Specific Members ---
  std::unique_ptr<detail::DiskManager<T, SLAB_SIZE>> m_DiskManager;
  std::size_t m_ActiveSlabs = 0;
  std::size_t m_MaxRamSlabs = 1000;

  CUDA_HOST_DEVICE T* sbo_elements() {
#if __cplusplus >= 201703L
    return std::launder(reinterpret_cast<T*>(m_static_buffer));
#else
    return reinterpret_cast<T*>(m_static_buffer);
#endif
  }

  CUDA_HOST_DEVICE const T* sbo_elements() const {
#if __cplusplus >= 201703L
    return std::launder(reinterpret_cast<const T*>(m_static_buffer));
#else
    return reinterpret_cast<const T*>(m_static_buffer);
#endif
  }

  void check_and_evict() {
    if (m_ActiveSlabs >= m_MaxRamSlabs) {
      Slab* candidate = m_head;
      while (candidate && (candidate->is_on_disk || candidate == m_tail))
        candidate = candidate->next;
      if (candidate) {
        if (!m_DiskManager)
          m_DiskManager.reset(new detail::DiskManager<T, SLAB_SIZE>());
        candidate->disk_offset =
            m_DiskManager->write_slab(candidate->elements());
        candidate->deallocate();
        candidate->is_on_disk = true;
        m_ActiveSlabs--;
      }
    }
  }

  void ensure_loaded(Slab* slab) {
    if (slab && slab->is_on_disk) {
      if (m_ActiveSlabs >= m_MaxRamSlabs) {
        Slab* v = m_head;
        while (v) {
          if (!v->is_on_disk && v != slab) {
            v->disk_offset = m_DiskManager->write_slab(v->elements());
            v->deallocate();
            v->is_on_disk = true;
            m_ActiveSlabs--;
            break;
          }
          v = v->next;
        }
      }
      slab->allocate();
      m_DiskManager->read_slab(slab->elements(), slab->disk_offset);
      slab->is_on_disk = false;
      m_ActiveSlabs++;
    }
  }

public:
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using iterator = tape_iterator<T, SBO_SIZE, SLAB_SIZE, is_multithread, true>;
  using const_iterator =
      tape_iterator<const T, SBO_SIZE, SLAB_SIZE, is_multithread, true>;

#ifndef __CUDACC__
  std::mutex& mutex() const { return m_TapeMutex; }
#endif

  CUDA_HOST_DEVICE tape_impl() = default;
  CUDA_HOST_DEVICE ~tape_impl() { clear(); }

  template <typename... ArgsT>
  CUDA_HOST_DEVICE void emplace_back(ArgsT&&... args) {
    if (m_size < SBO_SIZE) {
      ::new (const_cast<void*>(static_cast<const volatile void*>(
          sbo_elements() + m_size))) T(std::forward<ArgsT>(args)...);
    } else {
      const auto offset = (m_size - SBO_SIZE) % SLAB_SIZE;
      if (!offset) {
        if (m_size == m_capacity) {
          check_and_evict();
          Slab* new_slab = new Slab();
          m_ActiveSlabs++;
          if (!m_head)
            m_head = new_slab;
          else {
            m_tail->next = new_slab;
            new_slab->prev = m_tail;
          }
          m_capacity += SLAB_SIZE;
        }
        if (m_size == SBO_SIZE)
          m_tail = m_head;
        else
          m_tail = m_tail->next;
      }
      ensure_loaded(m_tail);
      ::new (const_cast<void*>(static_cast<const volatile void*>(
          m_tail->elements() + offset))) T(std::forward<ArgsT>(args)...);
    }
    m_size++;
  }

  // ... (Rest of functions are mostly same, just calling ensure_loaded/check_and_evict)
  CUDA_HOST_DEVICE std::size_t size() const { return m_size; }
  CUDA_HOST_DEVICE iterator begin() { return iterator(this, 0); }
  CUDA_HOST_DEVICE const_iterator begin() const {
    return const_iterator(this, 0);
  }
  CUDA_HOST_DEVICE iterator end() { return iterator(this, m_size); }
  CUDA_HOST_DEVICE const_iterator end() const {
    return const_iterator(this, m_size);
  }

  CUDA_HOST_DEVICE reference back() {
    assert(m_size);
    return (*this)[m_size - 1];
  }

  CUDA_HOST_DEVICE const_reference back() const {
    assert(m_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<tape_impl*>(this)->operator[](m_size - 1);
  }

  CUDA_HOST_DEVICE reference operator[](std::size_t i) {
    assert(i < m_size);
    return *at(i);
  }

  CUDA_HOST_DEVICE const_reference operator[](std::size_t i) const {
    assert(i < m_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return *const_cast<tape_impl*>(this)->at(i);
  }

  CUDA_HOST_DEVICE void pop_back() {
    assert(m_size);
    m_size--;
    if (m_size < SBO_SIZE)
      destroy_element(sbo_elements() + m_size);
    else {
      ensure_loaded(m_tail);
      std::size_t offset = (m_size - SBO_SIZE) % SLAB_SIZE;
      destroy_element(m_tail->elements() + offset);
      if (offset == 0) {
        if (m_tail != m_head)
          m_tail = m_tail->prev;
      }
    }
  }

private:
  CUDA_HOST_DEVICE T* at(std::size_t index) {
    if (index < SBO_SIZE)
      return sbo_elements() + index;
    Slab* slab = m_head;
    std::size_t idx = (index - SBO_SIZE) / SLAB_SIZE;
    while (idx--)
      slab = slab->next;
    ensure_loaded(slab);
    return slab->elements() + ((index - SBO_SIZE) % SLAB_SIZE);
  }

  template <typename It> using value_type_of = decltype(*std::declval<It>());
  template <typename It>
  static typename std::enable_if<
      !std::is_trivially_destructible<value_type_of<It>>::value>::type
  destroy(It B, It E) {
    for (It I = E - 1; I >= B; --I)
      I->~value_type_of<It>();
  }
  template <typename It>
  static typename std::enable_if<
      std::is_trivially_destructible<value_type_of<It>>::value>::type
      CUDA_HOST_DEVICE
      destroy(It B, It E) {}

  void clear() {
    std::size_t count = m_size;
    for (std::size_t i = 0; i < SBO_SIZE && count > 0; ++i, --count)
      destroy_element(&sbo_elements()[i]);
    Slab* slab = m_head;
    while (slab) {
      size_t current_slab_count = (count > SLAB_SIZE) ? SLAB_SIZE : count;
      count -= current_slab_count;
      Slab* tmp = slab;
      slab = slab->next;
      delete tmp;
    }
    m_head = nullptr;
    m_tail = nullptr;
    m_size = 0;
    m_capacity = SBO_SIZE;
    m_ActiveSlabs = 0;
  }

  template <typename ElTy> void destroy_element(ElTy* elem) { elem->~ElTy(); }
  template <typename ElTy, size_t N> void destroy_element(ElTy (*arr)[N]) {
    for (size_t i = 0; i < N; ++i)
      (*arr)[i].~ElTy();
  }
};

} // namespace clad

#endif // CLAD_TAPE_H