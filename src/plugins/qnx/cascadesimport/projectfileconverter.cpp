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

#include "projectfileconverter.h"

#include <qnx/qnxconstants.h>

#include <coreplugin/generatedfile.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QSet>

namespace Qnx {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// ProjectFileConverter
//
//////////////////////////////////////////////////////////////////////////////
ProjectFileConverter::ProjectFileConverter(ConvertedProjectContext &ctx)
: FileConverter(ctx)
{
}

static QMap<QString, QString> scanForDefinedVariables(const Core::GeneratedFile &file)
{
    QMap<QString, QString> ret;
    QString origContent = file.contents();
    QStringList lines = origContent.split(QLatin1Char('\n'), QString::KeepEmptyParts);
    QString line;
    foreach (QString ln, lines) {
        ln = ln.trimmed();
        if (ln.length() && (ln.at(0) == QLatin1Char('#')))
            continue;
        if ((line.length() > 0) && (line.at(line.length() - 1) == QLatin1Char('\\'))) {
            line = line.mid(line.length() - 1);
            line += ln;
        } else {
            line = ln;
        }
        if ((line.length() > 0) && (line.at(line.length() - 1) != QLatin1Char('\\'))) {
            int ix1 = line.indexOf(QLatin1Char('='));
            if (ix1 > 0) {
                int ix2 = (line.at(ix1 - 1) == QLatin1Char('+'))? ix1 - 1: ix1;
                QString k = line.mid(0, ix2).trimmed().toUpper();
                QString v = line.mid(ix1 + 1).trimmed();
                if (ix1 != ix2)
                    v = ret.value(k) + QLatin1Char(' ') + v;
                ret[k] = v;
            }
        }
    }
    return ret;
}

static QSet<QString> parseVariable(const QString &varStr)
{
    QStringList sl = varStr.split(QLatin1Char(' '), QString::SkipEmptyParts);
    QSet<QString> ret = QSet<QString>::fromList(sl);
    return ret;
}

static QString updateVariable(const QString &varStr, const QString &varsToAdd,
                              const QString &varsToRemove)
{
    QSet<QString> var = parseVariable(varStr);

    QSet<QString> ss = parseVariable(varsToAdd);
    foreach (QString s, ss)
        var << s;

    ss = parseVariable(varsToRemove);
    foreach (QString s, ss)
        var.remove(s);

    QStringList sl = QStringList::fromSet(var);
    return sl.join(QLatin1String(" "));
}

bool ProjectFileConverter::convertFile(Core::GeneratedFile &file, QString &errorMessage)
{
    if (!FileConverter::convertFile(file, errorMessage))
        return false;

    ImportLog &log = convertedProjectContext().importLog();
    QMap<QString, QString> origProjectVariables = scanForDefinedVariables(file);
    QString fileContent;

    QLatin1String path( ":/qnx/cascadesimport/resources/templates/project.pro");
    QByteArray ba = loadFileContent(path, errorMessage);
    if (!errorMessage.isEmpty())
        return false;
    fileContent = QString::fromUtf8(ba);

    QStringList headers;
    QStringList sources;
    QStringList resources;
    QStringList otherFiles;

    foreach (const QString &filePath, convertedProjectContext().collectedFiles()) {
        QString ext = filePath.section(QLatin1Char('.'), -1);
        if (ext.compare(QLatin1String("c"), Qt::CaseInsensitive) == 0)
            sources << filePath;
        else if (ext.compare(QLatin1String("cc"), Qt::CaseInsensitive) == 0)
            sources << filePath;
        else if (ext.compare(QLatin1String("cpp"), Qt::CaseInsensitive) == 0)
            sources << filePath;
        else if (ext.compare(QLatin1String("h"), Qt::CaseInsensitive) == 0)
            headers << filePath;
        else if (ext.compare(QLatin1String("hh"), Qt::CaseInsensitive) == 0)
            headers << filePath;
        else if (ext.compare(QLatin1String("hpp"), Qt::CaseInsensitive) == 0)
            headers << filePath;
        else if (ext.compare(QLatin1String("qrc"), Qt::CaseInsensitive) == 0)
            resources << filePath;
        else if (ext.compare(QLatin1String("qml"), Qt::CaseInsensitive) == 0)
            otherFiles << filePath;
        else if (ext.compare(QLatin1String("js"), Qt::CaseInsensitive) == 0)
            otherFiles << filePath;
        else if (ext.compare(QLatin1String("ts"), Qt::CaseInsensitive) == 0)
            otherFiles << filePath;
        else if (ext.compare(QLatin1String("pro"), Qt::CaseInsensitive) == 0)
            otherFiles << filePath;
        else if (filePath.compare(QLatin1String("bar-descriptor.xml"), Qt::CaseInsensitive) == 0)
            otherFiles << filePath;
        else if (filePath.startsWith(QLatin1String("assets/"), Qt::CaseInsensitive))
            // include all the content of the assets directory
            otherFiles << filePath;
        else if (filePath.startsWith(QLatin1String("src/"), Qt::CaseInsensitive))
            // include all the content of the src directory
            otherFiles << filePath;
        else if (ext.compare(QLatin1String("log"), Qt::CaseInsensitive) != 0)
                log.logWarning(tr("File \"%1\" not listed in \"%2\" file, should it be?")
                        .arg(filePath).arg(file.path()));
    }

    fileContent.replace(QLatin1String("%HEADERS%"), headers.join(QLatin1String(" \\\n    ")));
    fileContent.replace(QLatin1String("%SOURCES%"), sources.join(QLatin1String(" \\\n    ")));
    fileContent.replace(QLatin1String("%RESOURCES%"), resources.join(QLatin1String(" \\\n    ")));
    fileContent.replace(QLatin1String("%OTHER_FILES%"), otherFiles.join(QLatin1String(" \\\n    ")));
    fileContent.replace(QLatin1String("%PROJECT_NAME%"), convertedProjectContext().projectName());
    fileContent.replace(QLatin1String("%TARGET%"), origProjectVariables.value(QLatin1String("TARGET"),
                                      convertedProjectContext().projectName()));
    fileContent.replace(QLatin1String("%EXTRA_LIBS%"), origProjectVariables.value(QLatin1String("LIBS")));
    fileContent.replace(QLatin1String("%IMPORTER_VERSION%"),
                        QLatin1String(Qnx::Constants::QNX_BLACKBERRY_CASCADESIMPORTER_VERSION));
    fileContent.replace(QLatin1String("%DATE_TIME%"),
                                      QDateTime::currentDateTime().toString(Qt::ISODate));
    fileContent.replace(QLatin1String("%QT%"),
            updateVariable(origProjectVariables.value(QLatin1String("QT")),
                           QLatin1String("declarative script svg sql network xml xmlpatterns"),
                           QString()
    ));
    fileContent.replace(QLatin1String("%CONFIG%"),
            updateVariable(origProjectVariables.value(QLatin1String("CONFIG")),
                           QLatin1String("qt warn_on"),
                           QLatin1String("debug release debug_and_release cascades cascades10")
    ));
    fileContent.replace(QLatin1String("%MOBILITY%"), origProjectVariables.value(QLatin1String("MOBILITY")));
    file.setContents(fileContent);
    file.setAttributes(file.attributes() | Core::GeneratedFile::OpenProjectAttribute);
    return errorMessage.isEmpty();
}

} // namespace Internal
} // namespace Qnx
