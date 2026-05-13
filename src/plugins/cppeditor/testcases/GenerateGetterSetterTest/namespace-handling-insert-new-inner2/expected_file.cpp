#include "file.h"
namespace N2 {} // decoy
namespace {
namespace N1 {

namespace N2 {
int Something::getIt() const
{
    return it;
}

void Something::setIt(int value)
{
    it = value;
}

}

namespace {
}
}
}
