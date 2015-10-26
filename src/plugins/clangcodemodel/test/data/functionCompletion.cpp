class QString;
template <typename T> class TType { T t; };

void f();
void f(int a);
void f(const QString &s);
void f(char c, int optional = 3);
void f(char c, int optional1 = 3, int optional2 = 3);
void f(const TType<QString> *t);
TType<QString> f(bool);

void g()
{
    f( /* COMPLETE HERE */
}
