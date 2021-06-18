static void friendFunc();

class C {
    friend void friendFunc();
public:
    void publicFunc() {};

private:
    void privateFunc() {}
};

void friendFunc()
{
    C().p /* COMPLETE HERE */
}
