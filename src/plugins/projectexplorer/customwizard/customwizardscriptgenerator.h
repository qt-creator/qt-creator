/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CUSTOMWIZARDSCRIPTGENERATOR_H
#define CUSTOMWIZARDSCRIPTGENERATOR_H

#include <QtCore/QMap>
#include <QtCore/QStringList>

namespace Core {
class GeneratedFile;
}

namespace ProjectExplorer {
namespace Internal {

struct GeneratorScriptArgument;

/* Custom wizard script generator functions. In addition to the <file> elements
 * that define template files in which macros are replaced, it is possible to have
 * a custom wizard call a generation script (specified in the "generatorscript"
 * attribute of the <files> element) which actually creates files.
 * The command line of the script must follow the convention
 *
 * script [--dry-run] [options]
 *
 * Options containing field placeholders are configured in the XML files
 * and will be passed with them replaced by their values.
 *
 * As Qt Creator needs to know the file names before actually creates them to
 * do overwrite checking etc., this is  2-step process:
 * 1) Determine file names and attributes: The script is called with the
 *    --dry-run option and the field values. It then prints the relative path
 *    names it intends to create followed by comma-separated attributes
 *    matching those of the <file> element, for example:
 *        myclass.cpp,openeditor
 * 2) The script is called with the parameters only in the working directory
 * and then actually creates the files. If that involves directories, the script
 * should create those, too.
 */

// Parse the script arguments apart and expand the binary.
QStringList fixGeneratorScript(const QString &configFile, QString attributeIn);

// Step 1) Do a dry run of the generation script to get a list of files on stdout
QList<Core::GeneratedFile>
    dryRunCustomWizardGeneratorScript(const QString &targetPath,
                                      const QStringList &script,
                                      const QList<GeneratorScriptArgument> &arguments,
                                      const QMap<QString, QString> &fieldMap,
                                      QString *errorMessage);

// Step 2) Generate files
bool runCustomWizardGeneratorScript(const QString &targetPath,
                                    const QStringList &script,
                                    const QList<GeneratorScriptArgument> &arguments,
                                    const QMap<QString, QString> &fieldMap,
                                    QString *errorMessage);

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMWIZARDSCRIPTGENERATOR_H
