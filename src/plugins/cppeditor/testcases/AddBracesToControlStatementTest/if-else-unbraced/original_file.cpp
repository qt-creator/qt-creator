void MyObject::f()
{
    @if (true)
        emit mySig();
    else
        emit otherSig();
}
