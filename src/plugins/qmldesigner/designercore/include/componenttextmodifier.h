// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "textmodifier.h"

namespace QmlDesigner {

class ComponentTextModifier: public TextModifier
{
    Q_OBJECT
public:
    ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset);
    ~ComponentTextModifier() override;

    void replace(int offset, int length, const QString& replacement) override;
    void move(const MoveInfo &moveInfo) override;
    void indent(int offset, int length) override;
    void indentLines(int startLine, int endLine) override;

    TextEditor::TabSettings tabSettings() const override;

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
    bool moveToComponent(int /* nodeOffset */, const QString & /* importData */) override
    { return false; }

private:
    void handleOriginalTextChanged();

    TextModifier *m_originalModifier;
    int m_componentStartOffset;
    int m_componentEndOffset;
    int m_rootStartOffset;
    int m_startLength;
    QString m_originalText;
};

} // namespace QmlDesigner
