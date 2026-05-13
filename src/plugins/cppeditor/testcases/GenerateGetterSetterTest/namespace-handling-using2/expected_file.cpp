#include "file.h"
using namespace N1::N2::N3;
using namespace N1::N2;
namespace N1 {
using namespace N3;

int Something::getIt() const
{
    return it;
}

void Something::setIt(int value)
{
    it = value;
}

}
