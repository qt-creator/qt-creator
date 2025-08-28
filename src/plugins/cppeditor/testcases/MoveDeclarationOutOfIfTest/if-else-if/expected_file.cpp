void f()
{
    if (Foo *foo = g()) {
        Bar *bar = x();
        if (bar) {
            h();
            j();
        }
    } else {
        i();
    }
}
