#ifndef FILE__H__DECLARED
#define FILE__H__DECLARED
class Foo
{
public:
    int bar@;
    void setBar(int value);
};

inline void Foo::setBar(int value)
{
    bar = value;
}
#endif
