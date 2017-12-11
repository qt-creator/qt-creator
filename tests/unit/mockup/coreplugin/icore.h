#include <QDir>

namespace Core {
namespace ICore {
inline static QString userResourcePath()
{
     return QDir::tempPath();
}
} // namespace ICore
} // namespace Core
