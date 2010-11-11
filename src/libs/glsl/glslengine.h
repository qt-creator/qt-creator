#ifndef GLSLENGINE_H
#define GLSLENGINE_H

#include "glsl.h"
#include <set>
#include <string>

namespace GLSL {

class GLSL_EXPORT Engine
{
public:
    Engine();
    ~Engine();

    const std::string *identifier(const std::string &s);
    const std::string *identifier(const char *s, int n);

private:
    std::set<std::string> _identifiers;
};

} // namespace GLSL

#endif // GLSLENGINE_H
