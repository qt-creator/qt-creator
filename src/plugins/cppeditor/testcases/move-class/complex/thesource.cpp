#include "theheader.h"

namespace Project {
namespace Internal {

static void someFunction() {}

class TheClass::Private
{
    void func();
    int m_member = 0;
};

void TheClass::defined() {}

void TheClass::Private::func() {}

}
}
