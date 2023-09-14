// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "textmodifier.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class ChangeSet; }

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT PlainTextEditModifier: public TextModifier
{
    Q_OBJECT

private:
    PlainTextEditModifier(const PlainTextEditModifier &);
    PlainTextEditModifier &operator=(const PlainTextEditModifier &);

public:
    PlainTextEditModifier(QPlainTextEdit *textEdit);
    PlainTextEditModifier(QTextDocument *document, const QTextCursor &textCursor);
    ~PlainTextEditModifier() override;

    QTextDocument *textDocument() const override;
    QString text() const override;
    QTextCursor textCursor() const override;

    void replace(int offset, int length, const QString &replacement) override;
    void move(const MoveInfo &moveInfo) override;
    void indent(int offset, int length) override = 0;
    void indentLines(int startLine, int endLine) override = 0;

    void startGroup() override;
    void flushGroup() override;
    void commitGroup() override;

    void deactivateChangeSignals() override;
    void reactivateChangeSignals() override;

    bool renameId(const QString & /* oldId */, const QString & /* newId */) override
    { return false; }

    QStringList autoComplete(QTextDocument * /*textDocument*/, int /*position*/,  bool /*explicitComplete*/) override
    { return {}; }

    bool moveToComponent(int /* nodeOffset */, const QString & /* importData */) override
    { return false; }

private:
    void textEditChanged();
    void runRewriting(Utils::ChangeSet *writer);

private:
    Utils::ChangeSet *m_changeSet = nullptr;
    QTextDocument *m_textDocument;
    QTextCursor m_textCursor;
    bool m_changeSignalsEnabled{true};
    bool m_pendingChangeSignal{false};
    bool m_ongoingTextChange{false};
};

class QMLDESIGNERCORE_EXPORT NotIndentingTextEditModifier: public PlainTextEditModifier
{
public:
    NotIndentingTextEditModifier(QPlainTextEdit *textEdit)
        : PlainTextEditModifier(textEdit)
    {
        m_tabSettings.m_tabSize = 0;
        m_tabSettings.m_indentSize = 0;
    }

    NotIndentingTextEditModifier(QTextDocument *document, const QTextCursor &textCursor)
        : PlainTextEditModifier{document, textCursor}
    {
        m_tabSettings.m_tabSize = 0;
        m_tabSettings.m_indentSize = 0;
    }

    void indent(int /*offset*/, int /*length*/) override
    {}
    void indentLines(int /*offset*/, int /*length*/) override
    {}

    TextEditor::TabSettings tabSettings() const override
    { return m_tabSettings; }

protected:
    TextEditor::TabSettings m_tabSettings;
};

class QMLDESIGNERCORE_EXPORT IndentingTextEditModifier : public NotIndentingTextEditModifier
{
public:
    IndentingTextEditModifier(QTextDocument *document, const QTextCursor &textCursor);

    void indent(int offset, int length) override;
    void indentLines(int startLine, int endLine) override;
};

}
