#ifndef THE_HEADER_H
#define THE_HEADER_H

namespace Project {
namespace Internal {

class SomeClass {};
class TheClass
{
public:
    TheClass() = default;

    void defined();
    void undefined();

    template<typename T> T defaultValue() const;
private:
    class Private;
    class Undefined;
    static inline bool doesNotNeedDefinition = true;
    static bool needsDefinition;
    int m_value = 0;
};

template<typename T> T TheClass::defaultValue() const { return T(); }

}
}

#endif
