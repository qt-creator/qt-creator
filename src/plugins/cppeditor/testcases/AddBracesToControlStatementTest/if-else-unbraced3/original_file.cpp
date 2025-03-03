void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else
        emit otherSig();
}
