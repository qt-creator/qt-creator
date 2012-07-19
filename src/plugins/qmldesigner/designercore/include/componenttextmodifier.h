/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef COMPONENTTEXTMODIFIER_H
#define COMPONENTTEXTMODIFIER_H

#include "textmodifier.h"

namespace QmlDesigner {

class ComponentTextModifier: public TextModifier
{
    Q_OBJECT
public:
    ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset);
    ~ComponentTextModifier();

    virtual void replace(int offset, int length, const QString& replacement);
    virtual void move(const MoveInfo &moveInfo);
    virtual void indent(int offset, int length);

    virtual int indentDepth() const;

    virtual void startGroup();
    virtual void flushGroup();
    virtual void commitGroup();

    virtual QTextDocument *textDocument() const;
    virtual QString text() const;
    virtual QTextCursor textCursor() const;

    virtual void deactivateChangeSignals();
    virtual void reactivateChangeSignals();

    virtual QmlJS::Snapshot getSnapshot() const;
    virtual QStringList importPaths() const;

    virtual bool renameId(const QString & /* oldId */, const QString & /* newId */) { return false; }

public slots:
    void contentsChange(int position, int charsRemoved, int charsAdded);

private:
    TextModifier *m_originalModifier;
    int m_componentStartOffset;
    int m_componentEndOffset;
    int m_rootStartOffset;
    int m_startLength;
};

} // namespace QmlDesigner

#endif // COMPONENTTEXTMODIFIER_H
