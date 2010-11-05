
struct QString
{
    void append(char ch);
};

template <typename _Tp> struct QList {
    const _Tp &at(int index);
};

struct QStringList: public QList<QString> {};

int main()
{
    QStringList l;
    l.at(0).append('a'); 
}
