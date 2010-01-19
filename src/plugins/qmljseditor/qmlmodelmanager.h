/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLMODELMANAGER_H
#define QMLMODELMANAGER_H

#include "qmlmodelmanagerinterface.h"

#include <qmljs/qmljsdocument.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>

namespace Core {
class ICore;
}

namespace QmlJSEditor {
namespace Internal {

class QmlModelManager: public QmlModelManagerInterface
{
    Q_OBJECT

public:
    QmlModelManager(QObject *parent = 0);

    virtual QmlJS::Snapshot snapshot() const;
    virtual void updateSourceFiles(const QStringList &files);

    void emitDocumentUpdated(QmlJS::Document::Ptr doc);

Q_SIGNALS:
    void projectPathChanged(const QString &projectPath);
    void aboutToRemoveFiles(const QStringList &files);

private Q_SLOTS:
    // this should be executed in the GUI thread.
    void onDocumentUpdated(QmlJS::Document::Ptr doc);

protected:
    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles);
    QMap<QString, QString> buildWorkingCopyList();

    static void parse(QFutureInterface<void> &future,
                      QMap<QString, QString> workingCopy,
                      QStringList files,
                      QmlModelManager *modelManager);

private:
    mutable QMutex m_mutex;
    Core::ICore *m_core;
    QmlJS::Snapshot _snapshot;

    QFutureSynchronizer<void> m_synchronizer;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLMODELMANAGER_H
