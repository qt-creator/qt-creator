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
    { return QStringList(); }

    bool moveToComponent(int /* nodeOffset */) override
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

private:
    TextEditor::TabSettings m_tabSettings;
};

}
