#include <thread>

namespace details {
using stored_type = unsigned int;
}

template <typename T>
struct intrusive_ptr {
  using element_type = T;

  template <class Y>
  friend struct intrusive_ptr;

  intrusive_ptr() noexcept = default;

  intrusive_ptr(T* p, bool add_ref = true) : obj_pointer(p) {
    if (add_ref && obj_pointer != nullptr)
      intrusive_ptr_add_ref(obj_pointer);
  }

  intrusive_ptr(intrusive_ptr const& r) : obj_pointer(r.obj_pointer) {
    if (obj_pointer != nullptr)
      intrusive_ptr_add_ref(obj_pointer);
  }
  template <class Y>
  intrusive_ptr(intrusive_ptr<Y> const& r)
      requires(std::is_convertible_v<Y*, T*>)
      : obj_pointer(r.obj_pointer) {
    if (obj_pointer != nullptr)
      intrusive_ptr_add_ref(obj_pointer);
  }

  intrusive_ptr(intrusive_ptr&& r) : obj_pointer(r.obj_pointer) {
    r.obj_pointer = nullptr;
  }
  template <class Y>
  intrusive_ptr(intrusive_ptr<Y>&& r) requires(std::is_convertible_v<Y*, T*>)
      : obj_pointer(r.obj_pointer) {
    r.obj_pointer = nullptr;
  }

  ~intrusive_ptr() {
    if (obj_pointer != nullptr)
      intrusive_ptr_release(obj_pointer);
  }

  intrusive_ptr& operator=(intrusive_ptr const& r) {
    if (this != &r)
      intrusive_ptr(r).swap(*this);

    return *this;
  }

  template <class Y>
  intrusive_ptr& operator=(intrusive_ptr<Y> const& r)
      requires(std::is_convertible_v<Y*, T*>) {
    intrusive_ptr(r).swap(*this);

    return *this;
  }

  intrusive_ptr& operator=(T* r) {
    intrusive_ptr(r).swap(*this);

    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& r) {
    if (this != &r)
      intrusive_ptr(std::move(r)).swap(*this);

    return *this;
  }
  template <class Y>
  intrusive_ptr& operator=(intrusive_ptr<Y>&& r)
      requires(std::is_convertible_v<Y*, T*>) {
    intrusive_ptr(std::move(r)).swap(*this);

    return *this;
  }

  void reset() {
    intrusive_ptr().swap(*this);
  }

  void reset(T* r) {
    intrusive_ptr(r).swap(*this);
  }
  void reset(T* r, bool add_ref) {
    intrusive_ptr(r, add_ref).swap(*this);
  }

  T& operator*() const noexcept {
    return *static_cast<T*>(obj_pointer);
  }

  T* operator->() const noexcept {
    return static_cast<T*>(obj_pointer);
  }

  T* get() const noexcept {
    return obj_pointer;
  }
  T* detach() noexcept {
    T* current_obj_pointer = obj_pointer;
    obj_pointer = nullptr;
    return current_obj_pointer;
  }
  //
  explicit operator bool() const noexcept {
    return obj_pointer != nullptr;
  }
  //
  void swap(intrusive_ptr& b) noexcept {
    std::swap(b.obj_pointer, this->obj_pointer);
  }

private:
  T* obj_pointer = nullptr;
};

template <class T, class U>
bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
  return a.get() != b.get();
}

template <class T, class U>
bool operator==(intrusive_ptr<T> const& a, U* b) noexcept {
  return a.get() == b;
}

template <class T, class U>
bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept {
  return a.get() != b;
}

template <class T, class U>
bool operator==(T* a, intrusive_ptr<U> const& b) noexcept {
  return a == b.get();
}

template <class T, class U>
bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept {
  return a != b.get();
}

template <class T>
bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) noexcept {
  return std::less<T*>()(a.get(), b.get());
}

template <class T>
void swap(intrusive_ptr<T>& a, intrusive_ptr<T>& b) noexcept {
  a.swap(b);
}

template <typename T>
struct intrusive_ref_counter {
private:
  using type = std::atomic<details::stored_type>;
  mutable type counter = 0;

public:
  intrusive_ref_counter() noexcept = default;
  intrusive_ref_counter(const intrusive_ref_counter&) noexcept {}

  intrusive_ref_counter& operator=(const intrusive_ref_counter&) noexcept {
    return *this;
  }

  unsigned int use_count() const noexcept {
    return counter.load(std::memory_order_acquire);
  }

protected:
  ~intrusive_ref_counter() = default;

private:
  template <class Derived>
  friend void
  intrusive_ptr_add_ref(const intrusive_ref_counter<Derived>* p) noexcept;
  template <class Derived>
  friend void
  intrusive_ptr_release(const intrusive_ref_counter<Derived>* p) noexcept;

  void inc() const noexcept {
    counter.fetch_add(1, std::memory_order_relaxed);
  }

  details::stored_type dec() const noexcept {
    return counter.fetch_sub(1, std::memory_order_acq_rel);
  }
};

template <class Derived>
void intrusive_ptr_add_ref(const intrusive_ref_counter<Derived>* p) noexcept {
  p->inc();
}

template <class Derived>
void intrusive_ptr_release(const intrusive_ref_counter<Derived>* p) noexcept {
  if (p->dec() == 1)
    delete static_cast<const Derived*>(p);
}
