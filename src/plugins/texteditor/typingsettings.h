/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TYPINGSETTINGS_H
#define TYPINGSETTINGS_H

#include "texteditor_global.h"

#include <QVariant>
#include <QTextBlock>

QT_BEGIN_NAMESPACE
class QSettings;
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
