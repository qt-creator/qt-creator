struct Bar{ QString *str; };
void foo() {
    Bar *@bar = new Bar;
    bar->str = new QString;
    delete bar->str;
    delete bar;
}
