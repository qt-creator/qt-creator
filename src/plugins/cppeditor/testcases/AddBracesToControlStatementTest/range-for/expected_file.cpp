void MyObject::f()
{
    for (int i : list) {
        emit mySig();
    }
}
