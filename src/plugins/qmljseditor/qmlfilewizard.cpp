/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlfilewizard.h"
#include "qmljseditorconstants.h"

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

    if (baseFileWizardParameters().id() == QLatin1String(Constants::WIZARD_QML1FILE))
        str << QLatin1String("import QtQuick 1.1\n");
    else
        str << QLatin1String("import QtQuick 2.0\n");

    // 100:62 is the 'golden ratio'
    str << QLatin1String("\n")
        << QLatin1String("Rectangle {\n")
        << QLatin1String("    width: 100\n")
        << QLatin1String("    height: 62\n")
        << QLatin1String("}\n");

    return contents;
}
