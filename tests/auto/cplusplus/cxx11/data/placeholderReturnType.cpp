struct S {
    decltype(auto)
    operator*() const { return S(); }
};
