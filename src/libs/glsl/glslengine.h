#ifndef GLSLENGINE_H
#define GLSLENGINE_H

#include "glsl.h"
#include "glslmemorypool.h"
#include <QtCore/qstring.h>
#include <QtCore/qset.h>

namespace GLSL {

class GLSL_EXPORT Engine
{
public:
    Engine();
    ~Engine();

    const QString *identifier(const QString &s);
    const QString *identifier(const char *s, int n);

    MemoryPool *pool();

private:
    QSet<QString> _identifiers;
    MemoryPool _pool;
};

} // namespace GLSL

#endif // GLSLENGINE_H
