#include "theheader.h"

namespace Project {
namespace Internal {

static void someFunction() {}

class TheClass::Private
{
    Private();
    void func();
    int m_member = 0;
};

void TheClass::defined() {}

TheClass::Private::Private() = default;

void TheClass::Private::func() {}

}
}
