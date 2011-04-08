/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MEMORYTOOLTIP_H
#define MEMORYTOOLTIP_H

#include "debuggerconstants.h"

#include <projectexplorer/abi.h>

#include <QtGui/QTextEdit> // QTextEdit::ExtraSelection
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QPlainTextEdit;
class QToolButton;
class QTextCharFormat;
QT_END_NAMESPACE

namespace Debugger {
class DebuggerEngine;
namespace Internal {
class RegisterHandler;

// Documentation inside.
class MemoryViewWidget : public QWidget
{
    Q_OBJECT
public:
    // Address range to be marked with special format
    struct Markup
    {
        Markup(quint64 a = 0, quint64 s = 0,
               const QTextCharFormat &fmt = QTextCharFormat(),
               const QString &toolTip = QString());
        bool covers(quint64 a) const { return a >= address && a < (address + size); }

        quint64 address;
        quint64 size;
        QTextCharFormat format;
        QString toolTip;
    };

    explicit MemoryViewWidget(QWidget *parent = 0);

    quint64 address() const                    { return m_address; }
    quint64 length() const                     { return m_length; }

     // How read an address used for 'dereference pointer at' context menu action
    void setAbi(const ProjectExplorer::Abi &a) { m_abi = a; }
    ProjectExplorer::Abi abi() const           { return m_abi; }

    bool updateOnInferiorStop() const          { return m_updateOnInferiorStop; }
    void setUpdateOnInferiorStop(bool v)       { m_updateOnInferiorStop = v ; }

    QTextCharFormat textCharFormat() const;

    QList<Markup> markup() const               { return m_markup; }
    void setMarkup(const QList<Markup> &m)     { clearMarkup(); m_markup = m; }

    static QString formatData(quint64 address, const QByteArray &d);

    static const quint64 defaultLength;

    virtual bool eventFilter(QObject *, QEvent *);

signals:
    // Fetch memory and use setData().
    void memoryRequested(quint64 address, quint64 length);
    // Open a (sub) view from context menu
    void openViewRequested(quint64 address, quint64 length, const QPoint &pos);

public slots:
    void setData(const QByteArray &a); // Set to empty to indicate non-available data
    void engineStateChanged(Debugger::DebuggerState s);
    void addMarkup(quint64 begin, quint64 size, const QTextCharFormat &,
                   const QString &toolTip = QString());
    void addMarkup(quint64 begin, quint64 size, const QColor &background,
                   const QString &toolTip = QString());
    void clear();
    void clearMarkup();
    void requestMemory();
    void requestMemory(quint64 address, quint64 length);

protected:
    virtual void updateTitle();
    void setTitle(const QString &);

private slots:
    void slotNext();
    void slotPrevious();
    void slotContextMenuRequested(const QPoint &pos);

private:
    void setBrowsingEnabled(bool);
    quint64 addressAt(const QPoint &textPos) const;
    bool addressToLineColumn(quint64 address, int *line = 0, int *column = 0,
                             quint64 *lineStart = 0) const;
    bool markUpToSelections(const Markup &r,
                            QList<QTextEdit::ExtraSelection> *extraSelections) const;
    int indexOfMarkup(quint64 address) const;

    QToolButton *m_previousButton;
    QToolButton *m_nextButton;
    QPlainTextEdit *m_textEdit;
    QLabel *m_content;
    quint64 m_address;
    quint64 m_length;
    quint64 m_requestedAddress;
    quint64 m_requestedLength;
    ProjectExplorer::Abi m_abi;
    QByteArray m_data;
    bool m_updateOnInferiorStop;
    QList<Markup> m_markup;
};

class LocalsMemoryViewWidget : public MemoryViewWidget
{
    Q_OBJECT
public:
    explicit LocalsMemoryViewWidget(QWidget *parent = 0);
    void init(quint64 variableAddress, quint64 size, const QString &name);

private:
    virtual void updateTitle();

    quint64 m_variableAddress;
    quint64 m_variableSize;
    QString m_variableName;
};

class RegisterMemoryViewWidget : public MemoryViewWidget
{
    Q_OBJECT
public:
    explicit RegisterMemoryViewWidget(QWidget *parent = 0);
    void init(int registerIndex, RegisterHandler *h);

private slots:
    void slotRegisterSet(const QModelIndex &);

private:
    virtual void updateTitle();
    void setRegisterAddress(quint64 a);

    int m_registerIndex;
    quint64 m_registerAddress;
    quint64 m_offset;
    QString m_registerName;
};

} // namespace Internal
} // namespace Debugger

#endif // MEMORYTOOLTIP_H
