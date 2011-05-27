#ifndef CPPCODESTYLESETTINGS_H
#define CPPCODESTYLESETTINGS_H

#include "cpptools_global.h"

#include <QtCore/QMetaType>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStyleSettings
{
public:
    CppCodeStyleSettings();

    bool indentBlockBraces;
    bool indentBlockBody;
    bool indentClassBraces;
    bool indentEnumBraces;
    bool indentNamespaceBraces;
    bool indentNamespaceBody;
    bool indentAccessSpecifiers;
    bool indentDeclarationsRelativeToAccessSpecifiers;
    bool indentFunctionBody;
    bool indentFunctionBraces;
    bool indentSwitchLabels;
    bool indentStatementsRelativeToSwitchLabels;
    bool indentBlocksRelativeToSwitchLabels;
    bool indentControlFlowRelativeToSwitchLabels;

    // false: if (a &&
    //            b)
    //            c;
    // true:  if (a &&
    //                b)
    //            c;
    // but always: while (a &&
    //                    b)
    //                 foo;
    bool extraPaddingForConditionsIfConfusingAlign;

    // false: a = a +
    //                b;
    // true:  a = a +
    //            b
    bool alignAssignments;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const CppCodeStyleSettings &rhs) const;
};

inline bool operator==(const CppCodeStyleSettings &s1, const CppCodeStyleSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const CppCodeStyleSettings &s1, const CppCodeStyleSettings &s2) { return !s1.equals(s2); }

} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::CppCodeStyleSettings)

#endif // CPPCODESTYLESETTINGS_H
