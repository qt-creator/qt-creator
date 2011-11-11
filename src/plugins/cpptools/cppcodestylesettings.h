/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
