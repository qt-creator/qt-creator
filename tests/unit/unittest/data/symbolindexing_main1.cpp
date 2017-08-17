void function();

void function()
{
    int x;
    x = 20;
}

template <typename T>
T templateFunction(T t)
{
    return t;
}

template <>
int templateFunction(int t)
{
    return t;
}

extern template double templateFunction<double>(double);
template double templateFunction<double>(double);

template<typename T>
using TemplateFunctionType = T(&)(T);


TemplateFunctionType<int> aliasToTemplateFunction = templateFunction<int>;

void f()
{
    aliasToTemplateFunction(1);
}

void f(int);
void f(double);
