/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CALLGRINDSTACKBROWSER_H
#define CALLGRINDSTACKBROWSER_H

#include "../valgrind_global.h"

#include <QObject>
#include <QStack>

namespace Valgrind {
namespace Callgrind {

class Function;
class StackBrowser;

class VALGRINDSHARED_EXPORT HistoryItem
{
public:
    HistoryItem(StackBrowser *stack = 0);
    virtual ~HistoryItem();
};

class VALGRINDSHARED_EXPORT FunctionHistoryItem : public HistoryItem
{
public:
    FunctionHistoryItem(const Function *function, StackBrowser *stack = 0);
    virtual ~FunctionHistoryItem();

    const Function *function() const { return m_function; }

private:
    const Function *m_function;
};

class VALGRINDSHARED_EXPORT StackBrowser : public QObject
{
    Q_OBJECT

public:
    explicit StackBrowser(QObject *parent = 0);
    virtual ~StackBrowser();

    void select(HistoryItem *item);
    HistoryItem *current() const;

    void clear();
    int size() const;

public Q_SLOTS:
    void goBack();

Q_SIGNALS:
    void currentChanged();

private:
    QStack<HistoryItem *> m_stack;
};

}
}

#endif // CALLGRINDSTACKBROWSER_H
