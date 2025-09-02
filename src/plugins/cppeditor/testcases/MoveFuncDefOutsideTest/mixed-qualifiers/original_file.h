struct Base {
    virtual auto func() const && noexcept -> void = 0;
};
struct Derived : public Base {
    auto @func() const && noexcept -> void override {}
