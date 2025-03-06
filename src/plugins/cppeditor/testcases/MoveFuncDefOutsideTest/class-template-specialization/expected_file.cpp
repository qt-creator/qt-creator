template<typename T> class base {};
template<>
class base<int>
{
public:
    void bar();
};

void base<int>::bar() {}
