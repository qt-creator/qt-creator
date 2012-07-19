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

#include "quickfix.h"

using namespace TextEditor;

QuickFixOperation::QuickFixOperation(int priority)
{
    setPriority(priority);
}

QuickFixOperation::~QuickFixOperation()
{
}

int QuickFixOperation::priority() const
{
    return _priority;
}

void QuickFixOperation::setPriority(int priority)
{
    _priority = priority;
}

QString QuickFixOperation::description() const
{
    return _description;
}

void QuickFixOperation::setDescription(const QString &description)
{
    _description = description;
}

QuickFixFactory::QuickFixFactory(QObject *parent)
    : QObject(parent)
{
}

QuickFixFactory::~QuickFixFactory()
{
}
