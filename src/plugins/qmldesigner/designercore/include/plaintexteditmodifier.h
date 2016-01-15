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

#ifndef PLAINTEXTEDITMODIFIER_H
#define PLAINTEXTEDITMODIFIER_H

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
    ~PlainTextEditModifier();

    virtual void save(QIODevice *device);

    virtual QTextDocument *textDocument() const;
    virtual QString text() const;
    virtual QTextCursor textCursor() const;

    virtual void replace(int offset, int length, const QString &replacement);
    virtual void move(const MoveInfo &moveInfo);
    virtual void indent(int offset, int length) = 0;

    virtual int indentDepth() const = 0;

    virtual void startGroup();
    virtual void flushGroup();
    virtual void commitGroup();

    virtual void deactivateChangeSignals();
    virtual void reactivateChangeSignals();

    virtual bool renameId(const QString & /* oldId */, const QString & /* newId */) { return false; }

protected:
    QPlainTextEdit *plainTextEdit() const
    { return m_textEdit; }

private slots:
    void textEditChanged();

private:
    void runRewriting(Utils::ChangeSet *writer);

private:
    Utils::ChangeSet *m_changeSet;
    QPlainTextEdit *m_textEdit;
    bool m_changeSignalsEnabled;
    bool m_pendingChangeSignal;
    bool m_ongoingTextChange;
};

class QMLDESIGNERCORE_EXPORT NotIndentingTextEditModifier: public PlainTextEditModifier
{
public:
    NotIndentingTextEditModifier(QPlainTextEdit *textEdit)
        : PlainTextEditModifier(textEdit)
    {}

    virtual void indent(int /*offset*/, int /*length*/)
    {}

    virtual int indentDepth() const
    { return 0; }
};

}

#endif // PLAINTEXTEDITMODIFIER_H
