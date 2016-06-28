/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QLocale)

namespace Utils { class MimeType; }

namespace QmlJSTools {
namespace Internal {

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);
    ~ModelManager() override;

    void delayedInitialization();
protected:
    QHash<QString, QmlJS::Dialect> languageForSuffix() const override;
    void writeMessageInternal(const QString &msg) const override;
    WorkingCopy workingCopyInternal() const override;
    void addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const override;
    ProjectInfo defaultProjectInfoForProject(ProjectExplorer::Project *project) const override;
private:
    void updateDefaultProjectInfo();
    void loadDefaultQmlTypeDescriptions();
    QHash<QString, QmlJS::Dialect> initLanguageForSuffix() const;
};

} // namespace Internal

QMLJSTOOLS_EXPORT void setupProjectInfoQmlBundles(QmlJS::ModelManagerInterface::ProjectInfo &projectInfo);

} // namespace QmlJSTools
