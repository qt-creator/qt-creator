
using namespace N1;
N2::test *Foo::getBar() const
{
    return bar.get();
}

void Foo::setBar(const custom<N2::test> &newBar)
{
    if (bar.equals(newBar))
        return;
    bar.assign(newBar);
    emit barChanged(bar.get());
}
