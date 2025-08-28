int foo(int a, int b) {return a + b;}
int bar(int a) {return a;}
void baz() {
    int a = foo(ba@r(), bar());
}
