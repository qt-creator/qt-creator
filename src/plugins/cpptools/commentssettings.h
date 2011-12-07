#ifndef COMMENTSSETTINGS_H
#define COMMENTSSETTINGS_H

#include "cpptools_global.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

class CPPTOOLS_EXPORT CommentsSettings
{
public:
    CommentsSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    bool equals(const CommentsSettings &other) const;

    bool m_enableDoxygen;
    bool m_generateBrief;
    bool m_leadingAsterisks;
};

inline bool operator==(const CommentsSettings &a, const CommentsSettings &b)
{ return a.equals(b); }

inline bool operator!=(const CommentsSettings &a, const CommentsSettings &b)
{ return !(a == b); }

} // namespace CppTools

#endif // COMMENTSSETTINGS_H
