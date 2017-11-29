#include <QDir>

namespace Core {
namespace ICore {
static QString userResourcePath()
{
     return QDir::tempPath();
}
} // namespace ICore
} // namespace Core
