struct S {

    static int value;
};
class C {
public:
    void setValue(int v) { S::value = v; }
};
