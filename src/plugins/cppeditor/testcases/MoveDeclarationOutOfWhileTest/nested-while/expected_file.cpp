void f()
{
    while (Foo *foo = g()) {
        Bar *bar;
        while ((bar = h()) != 0) {
            i();
            j();
        }
    }
}
