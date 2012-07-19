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

#ifndef PLAINTEXTEDITMODIFIER_H
#define PLAINTEXTEDITMODIFIER_H

#include "corelib_global.h"
#include "textmodifier.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils {
    class ChangeSet;
}

namespace QmlDesigner {

class CORESHARED_EXPORT PlainTextEditModifier: public TextModifier
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

    virtual QmlJS::Snapshot getSnapshot() const = 0;
    virtual QStringList importPaths() const = 0;

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

class CORESHARED_EXPORT NotIndentingTextEditModifier: public PlainTextEditModifier
{
public:
    NotIndentingTextEditModifier(QPlainTextEdit *textEdit)
        : PlainTextEditModifier(textEdit)
    {}

    virtual void indent(int /*offset*/, int /*length*/)
    {}

    virtual int indentDepth() const
    { return 0; }

    virtual QmlJS::Snapshot getSnapshot() const;

    virtual QStringList importPaths() const;
};

}

#endif // PLAINTEXTEDITMODIFIER_H
