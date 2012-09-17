void foo() noexcept;
class C {
    void foo() const noexcept final override;
    void foo() const & noexcept final override;
    void foo() const && noexcept final override;
    auto foo() const noexcept -> void final override;
    auto foo() const & noexcept -> void final override;
    auto foo() const && noexcept -> void final override;
    void foo();
    void foo() &;
    void foo() &&;
};

int main() {
    void (C::*p)() const;
    void (C::*p)() const &;
    void (C::*p)() const &&;
    void (C::*p)();
    void (C::*p)() &;
    void (C::*p)() &&;
}
