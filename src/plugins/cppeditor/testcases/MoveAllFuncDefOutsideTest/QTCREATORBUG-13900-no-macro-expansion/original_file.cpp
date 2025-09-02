#define FAKE_Q_OBJECT int bar() {return 5;}
class F@oo {
    FAKE_Q_OBJECT
    int f1()
    {
        return 1;
    }
};
