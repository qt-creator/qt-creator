namespace N1 {
namespace N2 {
struct test{};
}
template<typename T>
struct custom {
    void assign(const custom<T>&);
    bool equals(const custom<T>&);
    T* get();
};

class Foo
{
public:
    custom<N2::test> bar@;
    N2::test *getBar() const;
    void setBar(const custom<N2::test> &newBar);
signals:
    void barChanged(N2::test *bar);
private:
    Q_PROPERTY(N2::test *bar READ getBar NOTIFY barChanged FINAL)
};
}
