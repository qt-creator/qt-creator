/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include "myinterfaces.h"
#include "ui_main.h"

#include <aggregate.h>

#include <QtGui/QWidget>

class MyMain : public QWidget
{
    Q_OBJECT

public:
    MyMain(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MyMain();

    void add(IComboEntry *obj);

private slots:
    void select(int index);

private:
    Ui::mainClass ui;

    QList<IComboEntry *> m_entries;
};

#endif // MAIN_H
