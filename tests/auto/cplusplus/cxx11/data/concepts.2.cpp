template <typename _Tp, _Tp __v> struct integral_constant {
    static constexpr _Tp value = __v;
    using value_type = _Tp;
    using type = integral_constant<_Tp, __v>;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};
template<bool __v> using __bool_constant = integral_constant<bool, __v>;
using true_type =  __bool_constant<true>;
using false_type = __bool_constant<false>;
template<typename _Tp> struct is_void : public false_type { };
template<> struct is_void<void> : public true_type { };
template<> struct is_void<const void> : public true_type { };
template<> struct is_void<volatile void> : public true_type { };
template<> struct is_void<const volatile void> : public true_type { };
template<typename> struct is_array : public false_type { };
template<typename _Tp, char8_t _Size>
struct is_array<_Tp[_Size]> : public true_type { };
template<typename _Tp> struct is_array<_Tp[]> : public true_type { };
template<typename> struct is_const : public false_type { };
template<typename _Tp> struct is_const<_Tp const> : public true_type { };
template<typename _Tp>
struct is_function : public __bool_constant<!is_const<const _Tp>::value> { };
template<typename _Tp> struct is_function<_Tp&> : public false_type { };
template<typename _Tp> struct is_function<_Tp&&> : public false_type { };
template <typename _Tp> inline constexpr bool is_array_v = false;
template <typename _Tp> inline constexpr bool is_array_v<_Tp[]> = true;
template <typename _Tp, unsigned long long _Num>
inline constexpr bool is_array_v<_Tp[_Num]> = true;
template <typename> struct is_lvalue_reference : public false_type {};
template <typename _Tp> struct is_lvalue_reference<_Tp &> : public true_type {};
template <typename _Tp> inline constexpr bool is_lvalue_reference_v = false;
template <typename _Tp> inline constexpr bool is_lvalue_reference_v<_Tp &> = true;
template <typename _Tp, typename _Up> inline constexpr bool is_same_v = false;
template <typename _Tp> inline constexpr bool is_same_v<_Tp, _Tp> = true;
template<bool, typename _Tp = void> struct enable_if {};
template<typename _Tp> struct enable_if<true, _Tp> { using type = _Tp; };
template<bool _Cond, typename _Tp = void>
using __enable_if_t = typename enable_if<_Cond, _Tp>::type;
template<typename _Tp, typename...> using __first_t = _Tp;
template<typename... _Bn> auto __or_fn(int) -> __first_t<false_type,
                               __enable_if_t<!bool(_Bn::value)>...>;
template<typename... _Bn> auto __or_fn(...) -> true_type;
template<typename... _Bn> struct __or_ : decltype(__or_fn<_Bn...>(0)) { };
template<typename... _Tp> struct common_reference;
template<typename... _Tp> using common_reference_t = typename common_reference<_Tp...>::type;
template<> struct common_reference<> { };
template<typename _Tp> struct __declval_protector {
    static const bool __stop = false;
};
template<typename _Tp>
auto declval() noexcept -> decltype(__declval<_Tp>(0))
{
    static_assert(__declval_protector<_Tp>::__stop,
                  "declval() must not be used!");
    return __declval<_Tp>(0);
}
template<typename _Tp0> struct common_reference<_Tp0> { using type = _Tp0; };
template<typename _Tp, typename _Up> concept __same_as = is_same_v<_Tp, _Up>;
template<typename _Tp, typename _Up> concept same_as = __same_as<_Tp, _Up> && __same_as<_Up, _Tp>;
template<typename _From,
         typename _To,
         bool = __or_<is_void<_From>, is_function<_To>, is_array<_To>>::value>
struct __is_convertible_helper
{
    using type = typename is_void<_To>::type;
};
template<typename _From, typename _To>
struct is_convertible : public __is_convertible_helper<_From, _To>::type
{};
template<typename _From, typename _To>
inline constexpr bool is_convertible_v = is_convertible<_From, _To>::value;
template<typename _From, typename _To>
concept convertible_to = is_convertible_v<_From, _To>
                         && requires { static_cast<_To>(declval<_From>()); };
template<typename _Tp> struct remove_reference { using type = _Tp; };
template<typename _Tp> struct remove_reference<_Tp &> { using type = _Tp; };
template<typename _Tp> struct remove_reference<_Tp &&> { using type = _Tp; };
template<typename _Tp> using remove_reference_t = typename remove_reference<_Tp>::type;
template<typename _Tp> using __cref = const remove_reference_t<_Tp> &;
template<typename _Tp, typename _Up>
concept common_reference_with = same_as<common_reference_t<_Tp, _Up>, common_reference_t<_Up, _Tp>>
                                && convertible_to<_Tp, common_reference_t<_Tp, _Up>>
                                && convertible_to<_Up, common_reference_t<_Tp, _Up>>;
template<typename _Lhs, typename _Rhs>
concept assignable_from = is_lvalue_reference_v<_Lhs>
                          && common_reference_with<__cref<_Lhs>, __cref<_Rhs>>
                          && requires(_Lhs __lhs, _Rhs &&__rhs) {
    { __lhs = static_cast<_Rhs &&>(__rhs) } -> same_as<_Lhs>;
};
using size_t = unsigned long;
template<typename _Type>
struct __type_identity
{
    using type = _Type;
};
template<typename _Tp>
using __type_identity_t = typename __type_identity<_Tp>::type;
template<typename _Tp, size_t = sizeof(_Tp)>
constexpr true_type __is_complete_or_unbounded(__type_identity<_Tp>)
{
    return {};
}

template<typename _TypeIdentity, typename _NestedType = typename _TypeIdentity::type>
constexpr typename __or_<is_reference<_NestedType>,
                         is_function<_NestedType>,
                         is_void<_NestedType>,
                         __is_array_unknown_bounds<_NestedType>>::type
__is_complete_or_unbounded(_TypeIdentity)
{
    return {};
}

template<typename _Tp, typename = void>
struct __add_lvalue_reference_helper
{
    using type = _Tp;
};

template<typename _Tp>
struct __add_lvalue_reference_helper<_Tp, __void_t<_Tp &>>
{
    using type = _Tp &;
};

template<typename _Tp>
using __add_lval_ref_t = typename __add_lvalue_reference_helper<_Tp>::type;

template<typename _Tp, typename... _Args>
using __is_nothrow_constructible_impl = __bool_constant<__is_nothrow_constructible(_Tp, _Args...)>;
template<typename _Tp>
struct is_nothrow_copy_constructible
    : public __is_nothrow_constructible_impl<_Tp, __add_lval_ref_t<const _Tp>>
{
    static_assert(__is_complete_or_unbounded(__type_identity<_Tp>{}),
                  "template argument must be a complete class or an unbounded array");
};

template<typename _Tp, typename... _Args>
using __is_constructible_impl = __bool_constant<__is_constructible(_Tp, _Args...)>;
/// @endcond

/// is_constructible
template<typename _Tp, typename... _Args>
struct is_constructible : public __is_constructible_impl<_Tp, _Args...>
{
    static_assert(__is_complete_or_unbounded(__type_identity<_Tp>{}),
                  "template argument must be a complete class or an unbounded array");
};

template<typename _Tp>
inline constexpr bool is_copy_constructible_v = __is_constructible(_Tp, __add_lval_ref_t<const _Tp>);
template<typename _Tp>
inline constexpr bool is_nothrow_copy_constructible_v
    = __is_nothrow_constructible(_Tp, __add_lval_ref_t<const _Tp>);

template<typename _Tp, typename... _Args>
inline constexpr bool is_constructible_v = __is_constructible(_Tp, _Args...);

template<typename _Tp>
constexpr bool __destructible_impl = false;
template<typename _Tp>
requires requires(_Tp &__t) {
    { __t.~_Tp() } noexcept;
}
constexpr bool __destructible_impl<_Tp> = true;

template<typename _Tp>
constexpr bool __destructible = __destructible_impl<_Tp>;
template<typename _Tp>
constexpr bool __destructible<_Tp &> = true;
template<typename _Tp>
constexpr bool __destructible<_Tp &&> = true;
template<typename _Tp, size_t _Nm>
constexpr bool __destructible<_Tp[_Nm]> = __destructible<_Tp>;

template<typename _Tp>
concept destructible = __destructible<_Tp>;

/// [concept.constructible], concept constructible_from
template<typename _Tp, typename... _Args>
concept constructible_from = destructible<_Tp> && is_constructible_v<_Tp, _Args...>;

template<typename _Tp>
concept move_constructible = constructible_from<_Tp, _Tp> && convertible_to<_Tp, _Tp>;

template<typename _Tp>
concept movable = is_object_v<_Tp> && move_constructible<_Tp> && assignable_from<_Tp &, _Tp>
                  && swappable<_Tp>;

template<typename _Tp>
concept copy_constructible = move_constructible<_Tp> && constructible_from<_Tp, _Tp &>
                             && convertible_to<_Tp &, _Tp> && constructible_from<_Tp, const _Tp &>
                             && convertible_to<const _Tp &, _Tp>
                             && constructible_from<_Tp, const _Tp>
                             && convertible_to<const _Tp, _Tp>;

template<typename _Tp>
concept copyable = copy_constructible<_Tp> && movable<_Tp> && assignable_from<_Tp &, _Tp &>
                   && assignable_from<_Tp &, const _Tp &> && assignable_from<_Tp &, const _Tp>;

template<typename _Tp>
concept __boxable = copy_constructible<_Tp> && is_object_v<_Tp>;
template<__boxable _Tp>
struct __box : std::optional<_Tp>
{
    using std::optional<_Tp>::optional;

    constexpr __box() noexcept(is_nothrow_default_constructible_v<_Tp>)
        requires default_initializable<_Tp>
        : std::optional<_Tp>{std::in_place}
    {}

    __box(const __box &) = default;
    __box(__box &&) = default;

    using std::optional<_Tp>::operator=;

    constexpr __box &operator=(const __box &__that) noexcept(is_nothrow_copy_constructible_v<_Tp>)
        requires(!copyable<_Tp>) && copy_constructible<_Tp>
    {
        if (this != std::__addressof(__that)) {
            if ((bool) __that)
                this->emplace(*__that);
            else
                this->reset();
        }
        return *this;
    }
};
template<typename _Tp>
concept __boxable_copyable = copy_constructible<_Tp>
                             && (copyable<_Tp>
                                 || (is_nothrow_move_constructible_v<_Tp>
                                     && is_nothrow_copy_constructible_v<_Tp>) );
template<typename _Tp>
concept __boxable_movable = (!copy_constructible<_Tp>)
    && (movable<_Tp> || is_nothrow_move_constructible_v<_Tp>);

template<__boxable _Tp>
    requires __boxable_copyable<_Tp> || __boxable_movable<_Tp>
    struct __box<_Tp>
{
private:
    [[no_unique_address]] _Tp _M_value = _Tp();

public:
    __box()
        requires default_initializable<_Tp>
        = default;

    constexpr explicit __box(const _Tp &__t) noexcept(is_nothrow_copy_constructible_v<_Tp>)
        requires copy_constructible<_Tp>
        : _M_value(__t)
    {}

    constexpr explicit __box(_Tp &&__t) noexcept(is_nothrow_move_constructible_v<_Tp>)
        : _M_value(std::move(__t))
    {}

    template<typename... _Args>
    requires constructible_from<_Tp, _Args...>
        constexpr explicit __box(in_place_t,
                                 _Args &&...__args) noexcept(is_nothrow_constructible_v<_Tp, _Args...>)
        : _M_value(std::forward<_Args>(__args)...)
    {}

    __box(const __box &) = default;
    __box(__box &&) = default;
    __box &operator=(const __box &)
        requires copyable<_Tp>
        = default;
    __box &operator=(__box &&)
        requires movable<_Tp>
        = default;

    // When _Tp is nothrow_copy_constructible but not copy_assignable,
    // copy assignment is implemented via destroy-then-copy-construct.
    constexpr __box &operator=(const __box &__that) noexcept
        requires(!copyable<_Tp>) && copy_constructible<_Tp>
    {
        static_assert(is_nothrow_copy_constructible_v<_Tp>);
        if (this != std::__addressof(__that)) {
            _M_value.~_Tp();
            std::construct_at(std::__addressof(_M_value), *__that);
        }
        return *this;
    }

    constexpr __box &operator=(__box &&__that) noexcept
        requires(!movable<_Tp>)
    {
        static_assert(is_nothrow_move_constructible_v<_Tp>);
        if (this != std::__addressof(__that)) {
            _M_value.~_Tp();
            std::construct_at(std::__addressof(_M_value), std::move(*__that));
        }
        return *this;
    }

    constexpr bool has_value() const noexcept { return true; }
    constexpr _Tp &operator*() & noexcept { return _M_value; }
    constexpr const _Tp &operator*() const & noexcept { return _M_value; }
    constexpr _Tp &&operator*() && noexcept { return std::move(_M_value); }
    constexpr const _Tp &&operator*() const && noexcept { return std::move(_M_value); }
    constexpr _Tp *operator->() noexcept { return std::__addressof(_M_value); }
    constexpr const _Tp *operator->() const noexcept { return std::__addressof(_M_value); }
    template<copy_constructible _Tp>
    requires is_object_v<_Tp>
        class single_view : public view_interface<single_view<_Tp>>
    {
    public:
        single_view()
            requires default_initializable<_Tp>
            = default;

        constexpr explicit single_view(const _Tp &__t) noexcept(is_nothrow_copy_constructible_v<_Tp>)
            requires copy_constructible<_Tp>
            : _M_value(__t)
        {}

        constexpr explicit single_view(_Tp &&__t) noexcept(is_nothrow_move_constructible_v<_Tp>)
            : _M_value(std::move(__t))
        {}

        template<typename... _Args>
        requires constructible_from<_Tp, _Args...>
            constexpr explicit single_view(in_place_t, _Args &&...__args) noexcept(
                is_nothrow_constructible_v<_Tp, _Args...>)
            : _M_value{in_place, std::forward<_Args>(__args)...}
        {}

        constexpr _Tp *begin() noexcept { return data(); }
        constexpr const _Tp *begin() const noexcept { return data(); }
        constexpr _Tp *end() noexcept { return data() + 1; }
        constexpr const _Tp *end() const noexcept { return data() + 1; }
        static constexpr bool empty() noexcept { return false; }
        static constexpr size_t size() noexcept { return 1; }
        constexpr _Tp *data() noexcept { return _M_value.operator->(); }
        constexpr const _Tp *data() const noexcept { return _M_value.operator->(); }
    private:
        [[no_unique_address]] __detail::__box<_Tp> _M_value;
    };

    template<typename _Tp>
    single_view(_Tp) -> single_view<_Tp>;
};
