/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef MODELDEMO_H
#define MODELDEMO_H

#include <QObject>
#include <QDebug>
#include <QItemSelection>
#include <QModelIndex>

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/errorlistmodel.h>
#include <valgrind/xmlprotocol/stackmodel.h>
#include <valgrind/memcheck/memcheckrunner.h>

class ModelDemo : public QObject
{
    Q_OBJECT
public:
    explicit ModelDemo(Valgrind::Memcheck::MemcheckRunner *r, QObject *parent = 0)
        : QObject(parent)
        , runner(r)
    {
    }

    Valgrind::XmlProtocol::StackModel* stackModel;

public Q_SLOTS:
    void finished() {
        qDebug() << runner->errorString();
    }

    void selectionChanged(const QItemSelection &sel, const QItemSelection &) {
        if (sel.indexes().isEmpty())
            return;
        const QModelIndex idx = sel.indexes().first();
        const Valgrind::XmlProtocol::Error err = idx.data(Valgrind::XmlProtocol::ErrorListModel::ErrorRole).value<Valgrind::XmlProtocol::Error>();
        qDebug() << idx.row() << err.what();
        stackModel->setError(err);
    }


private:
    Valgrind::Memcheck::MemcheckRunner *runner;
};

#endif // MODELDEMO_H
