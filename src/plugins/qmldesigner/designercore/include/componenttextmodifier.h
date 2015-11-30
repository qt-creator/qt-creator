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
