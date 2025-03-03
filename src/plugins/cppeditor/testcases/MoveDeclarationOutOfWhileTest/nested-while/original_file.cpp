void f()
{
    while (Foo *foo = g()) {
        while (Bar *@bar = h()) {
            i();
            j();
        }
    }
}
