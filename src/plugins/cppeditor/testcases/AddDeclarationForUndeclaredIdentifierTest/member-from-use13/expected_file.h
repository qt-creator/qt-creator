struct S {

    static void setValue(int);
};
class C {
public:
    void setValue(int v) { S::setValue(v); }
};
