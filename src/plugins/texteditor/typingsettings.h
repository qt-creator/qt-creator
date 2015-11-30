/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TYPINGSETTINGS_H
#define TYPINGSETTINGS_H

#include "texteditor_global.h"

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
class QTextDocument;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT TypingSettings
{
public:
    // This enum must match the indexes of tabKeyBehavior widget
    enum TabKeyBehavior {
        TabNeverIndents = 0,
        TabAlwaysIndents = 1,
        TabLeadingWhitespaceIndents = 2
    };

    // This enum must match the indexes of smartBackspaceBehavior widget
    enum SmartBackspaceBehavior {
        BackspaceNeverIndents = 0,
        BackspaceFollowsPreviousIndents = 1,
        BackspaceUnindents = 2
    };

    TypingSettings();

    bool tabShouldIndent(const QTextDocument *document, QTextCursor cursor, int *suggestedPosition) const;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const TypingSettings &ts) const;

    bool m_autoIndent;
    TabKeyBehavior m_tabKeyBehavior;
    SmartBackspaceBehavior m_smartBackspaceBehavior;
};

inline bool operator==(const TypingSettings &t1, const TypingSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const TypingSettings &t1, const TypingSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TypingSettings)

#endif // TYPINGSETTINGS_H
