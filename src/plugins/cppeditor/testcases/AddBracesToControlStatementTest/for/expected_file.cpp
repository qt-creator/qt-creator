void MyObject::f()
{
    for (int i = 0; i < 10; ++i) {
        emit mySig();
    }
}
