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

#include "qmljspreviewrunner.h"

#include <coreplugin/icore.h>

#include <projectexplorer/runnables.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QMessageBox>
#include <QApplication>

namespace QmlJSEditor {
namespace Internal {

QmlJSPreviewRunner::QmlJSPreviewRunner(QObject *parent) :
    QObject(parent)
{
    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::HostOsInfo::pathListSeparator()
                               + QString::fromLocal8Bit(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));
}

bool QmlJSPreviewRunner::isReady() const
{
    return !m_qmlViewerDefaultPath.isEmpty();
}

void QmlJSPreviewRunner::run(const QString &filename)
{
    QString errorMessage;
    if (!filename.isEmpty()) {
        ProjectExplorer::StandardRunnable r;
        r.environment = Utils::Environment::systemEnvironment();
        r.runMode = ProjectExplorer::ApplicationLauncher::Gui;
        r.executable = m_qmlViewerDefaultPath;
        r.commandLineArguments = Utils::QtcProcess::quoteArg(filename);
        m_applicationLauncher.start(r);
    } else {
        errorMessage = tr("No file specified.");
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(Core::ICore::dialogParent(), tr("Failed to preview Qt Quick file"),
                             tr("Could not preview Qt Quick (QML) file. Reason:\n%1").arg(errorMessage));
}


} // namespace Internal
} // namespace QmlJSEditor
