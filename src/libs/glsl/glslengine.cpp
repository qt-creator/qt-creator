#include "glslengine.h"

using namespace GLSL;

Engine::Engine()
{
}

Engine::~Engine()
{
}

const std::string *Engine::identifier(const std::string &s)
{
    return &*_identifiers.insert(s).first;
}

const std::string *Engine::identifier(const char *s, int n)
{
    return &*_identifiers.insert(std::string(s, n)).first;
}

MemoryPool *Engine::pool()
{
    return &_pool;
}
