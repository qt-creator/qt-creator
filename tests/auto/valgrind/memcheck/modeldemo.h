/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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
