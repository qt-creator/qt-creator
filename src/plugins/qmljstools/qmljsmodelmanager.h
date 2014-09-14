/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSMODELMANAGER_H
#define QMLJSMODELMANAGER_H

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsqrcparser.h>
#include <qmljs/qmljsconstants.h>

#include <cplusplus/CppDocument.h>
#include <utils/qtcoverride.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QLocale)

namespace Core { class MimeType; }

namespace QmlJSTools {
namespace Internal {

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);
    ~ModelManager();

    void delayedInitialization();
protected:
    QHash<QString, QmlJS::Dialect> languageForSuffix() const QTC_OVERRIDE;
    void writeMessageInternal(const QString &msg) const QTC_OVERRIDE;
    WorkingCopy workingCopyInternal() const QTC_OVERRIDE;
    void addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const QTC_OVERRIDE;
    ProjectInfo defaultProjectInfoForProject(ProjectExplorer::Project *project) const QTC_OVERRIDE;
private slots:
    void updateDefaultProjectInfo();
private:
    void loadDefaultQmlTypeDescriptions();
    static bool matchesMimeType(const Core::MimeType &fileMimeType, const Core::MimeType &knownMimeType);
};

} // namespace Internal

QMLJSTOOLS_EXPORT void setupProjectInfoQmlBundles(QmlJS::ModelManagerInterface::ProjectInfo &projectInfo);

} // namespace QmlJSTools

#endif // QMLJSMODELMANAGER_H
