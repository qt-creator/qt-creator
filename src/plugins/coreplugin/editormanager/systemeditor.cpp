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

#include "systemeditor.h"

#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

using namespace Core;
using namespace Core::Internal;

SystemEditor::SystemEditor(QObject *parent) :
    IExternalEditor(parent)
{
}

QStringList SystemEditor::mimeTypes() const
{
    return QStringList() << QLatin1String("application/octet-stream");
}

QString SystemEditor::id() const
{
    return QLatin1String("CorePlugin.OpenWithSystemEditor");
}

QString SystemEditor::displayName() const
{
    return QLatin1String("System Editor");
}

bool SystemEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    QUrl url;
    url.setPath(fileName);
    url.setScheme(QLatin1String("file"));
    if (!QDesktopServices::openUrl(url)) {
        if (errorMessage)
            *errorMessage = tr("Could not open url %1.").arg(url.toString());
        return false;
    }
    return true;
}
