struct Bar{ QString *str; };
void foo() {
    Bar bar;
    bar.str = new QString;
    delete bar.str;
    // delete bar;
}
