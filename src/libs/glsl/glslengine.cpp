#include "glslengine.h"

using namespace GLSL;

Engine::Engine()
{
}

Engine::~Engine()
{
}

const QString *Engine::identifier(const QString &s)
{
    return &(*_identifiers.insert(s));
}

const QString *Engine::identifier(const char *s, int n)
{
    return &(*_identifiers.insert(QString::fromLatin1(s, n)));
}

MemoryPool *Engine::pool()
{
    return &_pool;
}
