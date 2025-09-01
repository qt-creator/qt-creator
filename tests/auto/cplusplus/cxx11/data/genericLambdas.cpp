template <typename T> concept C1 = true;
template <std::size_t N> concept C2 = true;
template <typename A, typename B> concept C3 = true;
int main()
{
    auto f = []<class T>(T a, auto&& b) { return a < b; };
    auto g = []<typename... Ts>(Ts&&... ts) { return foo(std::forward<Ts>(ts)...); };
    auto h = []<typename T1, C1 T2> requires C2<sizeof(T1) + sizeof(T2)>
        (T1 a1, T1 b1, T2 a2, auto a3, auto a4) requires C3<decltype(a4), T2> {
        };
}
