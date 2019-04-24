#include <QDir>

namespace Core {
namespace ICore {
inline static QString userResourcePath()
{
     return QDir::tempPath();
}

inline static QString cacheResourcePath()
{
    return QDir::tempPath();
}

inline static QString resourcePath()
{
    return QDir::tempPath();
}

} // namespace ICore
} // namespace Core
