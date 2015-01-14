/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#include "qmljspreviewrunner.h"

#include <coreplugin/icore.h>

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

    m_applicationLauncher.setEnvironment(Utils::Environment::systemEnvironment());
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
                                    Utils::QtcProcess::quoteArg(filename));

    } else {
        errorMessage = tr("No file specified.");
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(Core::ICore::dialogParent(), tr("Failed to preview Qt Quick file"),
                             tr("Could not preview Qt Quick (QML) file. Reason:\n%1").arg(errorMessage));
}


} // namespace Internal
} // namespace QmlJSEditor
