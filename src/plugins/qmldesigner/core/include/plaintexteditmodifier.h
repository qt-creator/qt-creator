/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

    virtual void startGroup();
    virtual void flushGroup();
    virtual void commitGroup();

    virtual void deactivateChangeSignals();
    virtual void reactivateChangeSignals();

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

}

#endif // PLAINTEXTEDITMODIFIER_H
