void f()
{
    if (Foo *foo = g()) {
        if (Bar *@bar = x()) {
            h();
            j();
        }
    } else {
        i();
    }
}
