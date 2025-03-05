struct Object
{
    Object(int){}
};
void func()
{
    Object *@obj = new Object{0};
}
