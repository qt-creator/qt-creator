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

#include "cpptoolsreuse.h"

#include <Symbols.h>
#include <CoreTypes.h>
#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>

#include <QSet>
#include <QTextDocument>
#include <QTextCursor>
#include <QStringRef>

using namespace CPlusPlus;

namespace CppTools {

static void moveCursorToStartOrEndOfIdentifier(QTextCursor *tc,
                                               QTextCursor::MoveOperation op,
                                               int posDiff = 0)
{
    QTextDocument *doc = tc->document();
    if (!doc)
        return;

    QChar ch = doc->characterAt(tc->position() - posDiff);
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
        tc->movePosition(op);
        ch = doc->characterAt(tc->position() - posDiff);
    }
}

void moveCursorToEndOfIdentifier(QTextCursor *tc)
{
    moveCursorToStartOrEndOfIdentifier(tc, QTextCursor::NextCharacter);
}

void moveCursorToStartOfIdentifier(QTextCursor *tc)
{
    moveCursorToStartOrEndOfIdentifier(tc, QTextCursor::PreviousCharacter, 1);
}

static bool isOwnershipRAIIName(const QString &name)
{
    static QSet<QString> knownNames;
    if (knownNames.isEmpty()) {
        // Qt
        knownNames.insert(QLatin1String("QScopedPointer"));
        knownNames.insert(QLatin1String("QScopedArrayPointer"));
        knownNames.insert(QLatin1String("QMutexLocker"));
        knownNames.insert(QLatin1String("QReadLocker"));
        knownNames.insert(QLatin1String("QWriteLocker"));
        // Standard C++
        knownNames.insert(QLatin1String("auto_ptr"));
        knownNames.insert(QLatin1String("unique_ptr"));
        // Boost
        knownNames.insert(QLatin1String("scoped_ptr"));
        knownNames.insert(QLatin1String("scoped_array"));
    }

    return knownNames.contains(name);
}

bool isOwnershipRAIIType(CPlusPlus::Symbol *symbol, const LookupContext &context)
{
    if (!symbol)
        return false;

    // This is not a "real" comparison of types. What we do is to resolve the symbol
    // in question and then try to match its name with already known ones.
    if (symbol->isDeclaration()) {
        Declaration *declaration = symbol->asDeclaration();
        const NamedType *namedType = declaration->type()->asNamedType();
        if (namedType) {
            ClassOrNamespace *clazz = context.lookupType(namedType->name(),
                                                         declaration->enclosingScope());
            if (clazz && !clazz->symbols().isEmpty()) {
                Overview overview;
                Symbol *symbol = clazz->symbols().at(0);
                return isOwnershipRAIIName(overview.prettyName(symbol->name()));
            }
        }
    }

    return false;
}

bool isValidIdentifier(const QString &s)
{
    const int length = s.length();
    for (int i = 0; i < length; ++i) {
        const QChar &c = s.at(i);
        if (i == 0) {
            if (!c.isLetter() && c != QLatin1Char('_'))
                return false;
        } else {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_'))
                return false;
        }
    }
    return true;
}

bool isQtKeyword(const QStringRef &text)
{
    switch (text.length()) {
    case 4:
        switch (text.at(0).toLatin1()) {
        case 'e':
            if (text == QLatin1String("emit"))
                return true;
            break;
        case 'S':
            if (text == QLatin1String("SLOT"))
                return true;
            break;
        }
        break;

    case 5:
        if (text.at(0) == QLatin1Char('s') && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == QLatin1Char('S') && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 's':
            if (text == QLatin1String("signals"))
                return true;
            break;
        case 'f':
            if (text == QLatin1String("foreach") || text ==  QLatin1String("forever"))
                return true;
            break;
        }
        break;

    default:
        break;
    }
    return false;
}

} // CppTools
