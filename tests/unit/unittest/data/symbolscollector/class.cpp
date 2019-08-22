class Class
{
  Class();
  Class(int);

  void foo();

  void bar() {}
};

void Class::foo()
{
    bar();
}
