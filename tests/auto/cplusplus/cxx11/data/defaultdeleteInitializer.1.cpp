class C {
  C() = default;
  C(const C &) = delete;
  C &operator=(const C &) = default;

  void foo() = delete;
  template <class T> void bar(T) = delete;
};
