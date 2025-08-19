constexpr bool is_constant_evaluated() noexcept
{
    if consteval { return true; } else { return false; }
}

constexpr bool is_runtime_evaluated() noexcept
{
    if not consteval { return true; } else { return false; }
}
