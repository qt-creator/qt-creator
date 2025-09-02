void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2)
        emit sig2();
}
