/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fileconverter.h"

#include <utils/fileutils.h>
#include <coreplugin/generatedfile.h>

namespace Qnx {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// ConvertedProjectContext
//
//////////////////////////////////////////////////////////////////////////////
QString ConvertedProjectContext::projectName() const
{
    return destProjectPath().section(QLatin1Char('/'), -1);
}

//////////////////////////////////////////////////////////////////////////////
//
// FileConverter
//
//////////////////////////////////////////////////////////////////////////////
bool FileConverter::convertFile(Core::GeneratedFile &file, QString &errorMessage)
{
    ImportLog &log = convertedProjectContext().importLog();
    log.logInfo(tr("===== Converting file: %1").arg(file.path()));
    loadFileContent(file, errorMessage);
    if (!errorMessage.isEmpty())
        logError(errorMessage);
    return errorMessage.isEmpty();
}

QByteArray FileConverter::loadFileContent(const QString &filePath, QString &errorMessage)
{
    QByteArray ret;
    Utils::FileReader fr;
    QString absFilePath = filePath;
    if (!filePath.startsWith(QLatin1String(":/"))) {
        const QString srcProjectPath = convertedProjectContext().srcProjectPath();
        absFilePath = srcProjectPath + QLatin1Char('/') + filePath;
    }
    fr.fetch(absFilePath);
    if (!fr.errorString().isEmpty())
        errorMessage = fr.errorString();
    else
        ret = fr.data();
    return ret;
}

bool FileConverter::loadFileContent(Core::GeneratedFile &file, QString &errorMessage)
{
    if (file.binaryContents().isEmpty()) {
        // virtual files have some content set already
        QString filePath = file.path();
        QByteArray ba = loadFileContent(filePath, errorMessage);
        file.setBinaryContents(ba);
    }
    return errorMessage.isEmpty();
}

void FileConverter::logError(const QString &errorMessage)
{
    if (!errorMessage.isEmpty())
        convertedProjectContext().importLog().logError(errorMessage);
}

} // namespace Internal
} // namespace Qnx

