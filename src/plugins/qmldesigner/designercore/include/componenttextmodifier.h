/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "textmodifier.h"

namespace QmlDesigner {

class ComponentTextModifier: public TextModifier
{
    Q_OBJECT
public:
    ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset);
    ~ComponentTextModifier();

    void replace(int offset, int length, const QString& replacement) override;
    void move(const MoveInfo &moveInfo) override;
    void indent(int offset, int length) override;
    void indentLines(int startLine, int endLine) override;

    int indentDepth() const override;

    void startGroup() override;
    void flushGroup() override;
    void commitGroup() override;

    QTextDocument *textDocument() const override;
    QString text() const override;
    QTextCursor textCursor() const override;

    void deactivateChangeSignals() override;
    void reactivateChangeSignals() override;

    bool renameId(const QString & /* oldId */, const QString & /* newId */) override
    { return false; }
    QStringList autoComplete(QTextDocument * textDocument, int position, bool explicitComplete) override
    { return m_originalModifier->autoComplete(textDocument, position, explicitComplete); }
    bool moveToComponent(int /* nodeOffset */) override
    { return false; }

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
