#include "../utils/fileutils.h"

#include <QDir>


namespace Core {
namespace ICore {
inline static Utils::FilePath userResourcePath()
{
     return Utils::FilePath::fromString(QDir::tempPath());
}

inline static Utils::FilePath cacheResourcePath(const QString & /*rel*/ = {})
{
    return Utils::FilePath::fromString(QDir::tempPath());
}

inline static Utils::FilePath resourcePath()
{
    return Utils::FilePath::fromString(QDir::tempPath());
}

} // namespace ICore
} // namespace Core
