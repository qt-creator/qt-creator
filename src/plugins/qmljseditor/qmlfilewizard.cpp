/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmlfilewizard.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <QFileInfo>
#include <QTextStream>

using namespace QmlJSEditor;

QmlFileWizard::QmlFileWizard(const BaseFileWizardParameters &parameters,
                             QObject *parent):
    Core::StandardFileWizard(parameters, parent)
{
}

Core::GeneratedFiles QmlFileWizard::generateFilesFromPath(const QString &path,
                                                          const QString &name,
                                                          QString * /*errorMessage*/) const

{
    const QString mimeType = QLatin1String(QmlJSTools::Constants::QML_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(mimeType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString QmlFileWizard::fileContents(const QString &) const
{
    QString contents;
    QTextStream str(&contents);

    // 100:62 is the 'golden ratio'
    str << QLatin1String("import QtQuick 1.1\n")
        << QLatin1String("\n")
        << QLatin1String("Rectangle {\n")
        << QLatin1String("    width: 100\n")
        << QLatin1String("    height: 62\n")
        << QLatin1String("}\n");

    return contents;
}
