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

#include "qmljspreviewrunner.h"

#include <projectexplorer/environment.h>
#include <utils/synchronousprocess.h>

#include <QtGui/QMessageBox>
#include <QtGui/QApplication>

#include <QDebug>

namespace QmlJSEditor {
namespace Internal {

QmlJSPreviewRunner::QmlJSPreviewRunner(QObject *parent) :
    QObject(parent)
{
    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::SynchronousProcess::pathSeparator()
                               + QString(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));

    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();
    m_applicationLauncher.setEnvironment(environment.toStringList());
}

bool QmlJSPreviewRunner::isReady() const
{
    return !m_qmlViewerDefaultPath.isEmpty();
}

void QmlJSPreviewRunner::run(const QString &filename)
{
    QString errorMessage;
    if (!filename.isEmpty()) {
        m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_qmlViewerDefaultPath,
                                    QStringList() << filename);

    } else {
        errorMessage = "No file specified.";
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(0, tr("Failed to preview Qt Quick file"),
                             tr("Could not preview Qt Quick (QML) file. Reason: \n%1").arg(errorMessage));
}


} // namespace Internal
} // namespace QmlJSEditor
