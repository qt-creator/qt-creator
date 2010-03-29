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

#include "helpviewer.h"
#include "helpconstants.h"
#include "helpmanager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>

#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>

#include <QtHelp/QHelpEngineCore>

using namespace Help::Internal;

QString HelpViewer::AboutBlankPage =
    QCoreApplication::translate("HelpViewer", "<title>about:blank</title>");

QString HelpViewer::PageNotFoundMessage =
    QCoreApplication::translate("HelpViewer", "<title>Error 404...</title><div "
    "align=\"center\"><br><br><h1>The page could not be found</h1><br><h3>'%1'"
    "</h3></div>");

bool HelpViewer::isLocalUrl(const QUrl &url)
{
    const QString &scheme = url.scheme();
    return scheme.isEmpty()
        || scheme == QLatin1String("file")
        || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("data")
        || scheme == QLatin1String("qthelp")
        || scheme == QLatin1String("about");
}

bool HelpViewer::canOpenPage(const QString &url)
{
    return url.endsWith(QLatin1String(".html"), Qt::CaseInsensitive)
        || url.endsWith(QLatin1String(".htm"), Qt::CaseInsensitive)
        || url == Help::Constants::AboutBlank;
}

bool HelpViewer::launchWithExternalApp(const QUrl &url)
{
    if (isLocalUrl(url)) {
        const QHelpEngineCore &helpEngine = Help::HelpManager::helpEngineCore();
        const QUrl &resolvedUrl = helpEngine.findFile(url);
        if (!resolvedUrl.isValid())
            return false;

        const QString& path = resolvedUrl.path();
        if (!canOpenPage(path)) {
            QTemporaryFile tmpTmpFile;
            if (!tmpTmpFile.open())
                return false;

            const QString &extension = QFileInfo(path).completeSuffix();
            QFile actualTmpFile(tmpTmpFile.fileName() % QLatin1String(".")
                % extension);
            if (!actualTmpFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
                return false;

            actualTmpFile.write(helpEngine.fileData(resolvedUrl));
            actualTmpFile.close();
            return QDesktopServices::openUrl(QUrl(actualTmpFile.fileName()));
        }
    } else if (url.scheme() == QLatin1String("http")) {
        return QDesktopServices::openUrl(url);
    }
    return false;
}

void HelpViewer::home()
{
    const QHelpEngineCore &engine = Help::HelpManager::helpEngineCore();
    QString homepage = engine.customValue(QLatin1String("HomePage"),
        QLatin1String("")).toString();

    if (homepage.isEmpty()) {
        homepage = engine.customValue(QLatin1String("DefaultHomePage"),
            Help::Constants::AboutBlank).toString();
    }

    setSource(homepage);
}

bool HelpViewer::handleForwardBackwardMouseButtons(QMouseEvent *event)
{
    if (event->button() == Qt::XButton1) {
        backward();
        return true;
    }
    if (event->button() == Qt::XButton2) {
        forward();
        return true;
    }

    return false;
}
