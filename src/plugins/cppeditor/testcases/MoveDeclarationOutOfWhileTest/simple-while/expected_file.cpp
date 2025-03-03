void f()
{
    Foo *foo;
    while ((foo = g()) != 0)
        j();
}
