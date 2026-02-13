// minimal
auto foo() -> enable_if_t<is_integral_v<int> || true>;

// in context
template<typename T>
static inline auto fromValue(const T &value)
    noexcept(std::is_nothrow_copy_constructible_v<T> && Private::CanUseInternalSpace<T>)
    -> std::enable_if_t<std::is_copy_constructible_v<T> && std::is_destructible_v<T>, QVariant>
{
    if constexpr (std::is_null_pointer_v<T>)
        return QVariant(QMetaType::fromType<std::nullptr_t>());
    else if constexpr (std::is_same_v<T, QVariant>)
        return value;
    else if constexpr (std::is_same_v<T, std::monostate>)
        return QVariant();
    return QVariant(QMetaType::fromType<T>(), std::addressof(value));
}
