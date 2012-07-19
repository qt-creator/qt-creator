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

#ifndef CALLGRINDSTACKBROWSER_H
#define CALLGRINDSTACKBROWSER_H

#include <QObject>
#include <QStack>

namespace Valgrind {
namespace Callgrind {

class Function;

class StackBrowser : public QObject
{
    Q_OBJECT

public:
    explicit StackBrowser(QObject *parent = 0);

    void select(const Function *item);
    const Function *current() const;
    void clear();
    bool hasPrevious() const { return !m_stack.isEmpty(); }
    bool hasNext() const { return !m_redoStack.isEmpty(); }

public slots:
    void goBack();
    void goNext();

signals:
    void currentChanged();

private:
    QStack<const Function *> m_stack;
    QStack<const Function *> m_redoStack;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // CALLGRINDSTACKBROWSER_H
