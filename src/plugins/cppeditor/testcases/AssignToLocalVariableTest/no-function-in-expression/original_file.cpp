int foo(int a) {return a;}
int bar() {return 1;}
void baz() {foo(@bar() + bar());}
