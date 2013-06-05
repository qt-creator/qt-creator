#define Q_DECL_EQ_DELETE
#define Q_DISABLE_COPY(Class) \
    Class(const Class &) Q_DECL_EQ_DELETE;\
    Class &operator=(const Class &) Q_DECL_EQ_DELETE;

class Test {
private:
    Q_DISABLE_COPY(Test)

public:
    Test();
};
