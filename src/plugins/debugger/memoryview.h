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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    explicit RegisterMemoryView(QWidget *binEditor, quint64 addr, const QByteArray &regName,
                                RegisterHandler *rh, QWidget *parent = 0);

    static QList<MemoryMarkup> registerMarkup(quint64 a, const QByteArray &regName);
    static QString title(const QByteArray &registerName, quint64 a = 0);

private:
    void onRegisterChanged(const QByteArray &name, quint64 value);
    void setRegisterAddress(quint64 v);

    QByteArray m_registerName;
    quint64 m_registerAddress;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_MEMORYVIEW_H
