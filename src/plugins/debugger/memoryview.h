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

#ifndef DEBUGGER_INTERNAL_MEMORYVIEW_H
#define DEBUGGER_INTERNAL_MEMORYVIEW_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Debugger {
namespace Internal {
class MemoryMarkup;
class RegisterHandler;

class MemoryView : public QWidget
{
    Q_OBJECT
public:
    explicit MemoryView(QWidget *binEditor, QWidget *parent = 0);

    static void setBinEditorRange(QWidget *w, quint64 address, int range, int blockSize);
    static void setBinEditorReadOnly(QWidget *w, bool readOnly);
    static void setBinEditorNewWindowRequestAllowed(QWidget *w, bool a);
    static void setBinEditorMarkup(QWidget *w, const QList<MemoryMarkup> &ml);
    static void binEditorSetCursorPosition(QWidget *w, int p);
    static void binEditorUpdateContents(QWidget *w);
    static void binEditorAddData(QWidget *w, quint64 addr, const QByteArray &a);

public slots:
    void updateContents();

protected:
    void setAddress(quint64 a);
    void setMarkup(const QList<MemoryMarkup> &m);

private:
    QWidget *m_binEditor;
};

class RegisterMemoryView : public MemoryView
{
    Q_OBJECT
public:
    explicit RegisterMemoryView(QWidget *binEditor, QWidget *parent = 0);

    void init(RegisterHandler *rh, int index);

    static QList<MemoryMarkup> registerMarkup(quint64 a, const QString &name);
    static QString title(const QString &registerName, quint64 a = 0);

private slots:
    void slotRegisterSet(const QModelIndex &index);

private:
    void setRegisterAddress(quint64 v);

    int m_registerIndex;
    QString m_registerName;
    quint64 m_registerAddress;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_MEMORYVIEW_H
