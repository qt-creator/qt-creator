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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef TEXTMODIFIER_H
#define TEXTMODIFIER_H

#include "qmldesignercorelib_global.h"

#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QTextCursor>
#include <QTextDocument>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT TextModifier: public QObject
{
    Q_OBJECT

private:
    TextModifier(const TextModifier &);
    TextModifier &operator=(const TextModifier &);

public:
    struct MoveInfo {
        int objectStart;
        int objectEnd;
        int leadingCharsToRemove;
        int trailingCharsToRemove;

        int destination;
        QString prefixToInsert;
        QString suffixToInsert;

        MoveInfo(): objectStart(-1), objectEnd(-1), leadingCharsToRemove(0), trailingCharsToRemove(0), destination(-1) {}
    };

public:
    TextModifier() {}
    virtual ~TextModifier() = 0;

    virtual void replace(int offset, int length, const QString& replacement) = 0;
    virtual void move(const MoveInfo &moveInfo) = 0;
    virtual void indent(int offset, int length) = 0;

    virtual int indentDepth() const = 0;

    virtual void startGroup() = 0;
    virtual void flushGroup() = 0;
    virtual void commitGroup() = 0;

    virtual QTextDocument *textDocument() const = 0;
    virtual QString text() const = 0;
    virtual QTextCursor textCursor() const = 0;

    virtual void deactivateChangeSignals() = 0;
    virtual void reactivateChangeSignals() = 0;

    static QmlJS::Snapshot qmljsSnapshot();

    virtual bool renameId(const QString &oldId, const QString &newId) = 0;

signals:
    void textChanged();

    void replaced(int offset, int oldLength, int newLength);
    void moved(const TextModifier::MoveInfo &moveInfo);
};

}

#endif // TEXTMODIFIER_H
