class X {
    X(X&&) noexcept;
};

X::X(X&&) = default;

int function()
{
}

