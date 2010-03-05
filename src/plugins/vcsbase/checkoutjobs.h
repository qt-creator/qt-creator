/**************************************************************************
**
** This file is part of Qt Creator
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

#ifndef CHECKOUTJOB_H
#define CHECKOUTJOB_H

#include "vcsbase_global.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE
class QStringList;
class QByteArray;
QT_END_NAMESPACE

namespace VCSBase {

struct ProcessCheckoutJobPrivate;

/* Abstract base class for a job creating an initial project checkout.
 * It should be something that runs in the background producing log
 * messages. */

class VCSBASE_EXPORT AbstractCheckoutJob : public QObject
{
    Q_OBJECT
public:
    virtual void start() = 0;
    virtual void cancel() = 0;

protected:
    explicit AbstractCheckoutJob(QObject *parent = 0);

signals:
    void succeeded();
    void failed(const QString &why);
    void output(const QString &what);
};

/* Convenience implementation using a QProcess. */

class VCSBASE_EXPORT ProcessCheckoutJob : public AbstractCheckoutJob
{
    Q_OBJECT
public:
    explicit ProcessCheckoutJob(const QString &binary,
                                const QStringList &args,
                                const QString &workingDirectory = QString(),
                                const QStringList &env = QStringList(),
                                QObject *parent = 0);
    virtual ~ProcessCheckoutJob();

    virtual void start();
    virtual void cancel();

private slots:
    void slotError(QProcess::ProcessError error);
    void slotFinished (int exitCode, QProcess::ExitStatus exitStatus);
    void slotOutput();

private:
    ProcessCheckoutJobPrivate *d;
};

} // namespace VCSBase

#endif // CHECKOUTJOB_H
