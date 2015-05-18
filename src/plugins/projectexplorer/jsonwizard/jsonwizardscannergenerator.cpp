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

#include "jsonwizardscannergenerator.h"

#include "../customwizard/customwizardpreprocessor.h"
#include "../projectexplorer.h"
#include "../iprojectmanager.h"
#include "jsonwizard.h"
#include "jsonwizardfactory.h"

#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QCoreApplication>
#include <QDir>
#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

bool JsonWizardScannerGenerator::setup(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::Internal::JsonWizard",
                                                    "Key is not an object.");
        return false;
    }

    QVariantMap gen = data.toMap();

    m_binaryPattern = gen.value(QLatin1String("binaryPattern")).toString();
    QStringList patterns = gen.value(QLatin1String("subdirectoryPatterns")).toStringList();
    foreach (const QString pattern, patterns) {
        QRegularExpression regexp(pattern);
        if (!regexp.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::Internal::JsonWizard",
                                                        "Pattern \"%1\" is no valid regular expression.");
            return false;
        }
        m_subDirectoryExpressions << regexp;
    }

    m_firstProjectOnly = gen.value(QLatin1String("firstProjectOnly"), QLatin1String("true")).toString();

    return true;
}

Core::GeneratedFiles JsonWizardScannerGenerator::fileList(Utils::MacroExpander *expander,
                                                          const QString &wizardDir,
                                                          const QString &projectDir,
                                                          QString *errorMessage)
{
    Q_UNUSED(wizardDir);
    errorMessage->clear();

    QDir project(projectDir);
    Core::GeneratedFiles result;
    Utils::MimeDatabase mdb;

    QRegularExpression binaryPattern;
    if (!m_binaryPattern.isEmpty()) {
        binaryPattern = QRegularExpression(expander->expand(m_binaryPattern));
        if (!binaryPattern.isValid()) {
            qWarning() << QCoreApplication::translate("ProjectExplorer::Internal::JsonWizard",
                                                      "ScannerGenerator: Binary pattern \"%1\" not valid.")
                          .arg(m_binaryPattern);
            return result;
        }
    }

    bool onlyFirst = JsonWizard::boolFromVariant(m_firstProjectOnly, expander);

    result = scan(project.absolutePath(), project);

    QList<IProjectManager *> projectManagers
            = ExtensionSystem::PluginManager::getObjects<IProjectManager>();

    int projectCount = 0;
    for (auto it = result.begin(); it != result.end(); ++it) {
        const QString relPath = project.relativeFilePath(it->path());
        it->setBinary(binaryPattern.match(relPath).hasMatch());

        Utils::MimeType mt = mdb.mimeTypeForFile(relPath);
        if (mt.isValid()) {
            bool found = Utils::anyOf(projectManagers, [mt](IProjectManager *m) {
                return mt.matchesName(m->mimeType());
            });
            if (found && !(onlyFirst && projectCount++))
                it->setAttributes(it->attributes() | Core::GeneratedFile::OpenProjectAttribute);
        }
    }

    return result;
}

bool JsonWizardScannerGenerator::matchesSubdirectoryPattern(const QString &path)
{
    foreach (const QRegularExpression &regexp, m_subDirectoryExpressions) {
        if (regexp.match(path).hasMatch())
            return true;
    }
    return false;
}

Core::GeneratedFiles JsonWizardScannerGenerator::scan(const QString &dir, const QDir &base)
{
    Core::GeneratedFiles result;
    QDir directory(dir);

    if (!directory.exists())
        return result;

    QList<QFileInfo> entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                       QDir::DirsLast | QDir::Name);
    foreach (const QFileInfo &fi, entries) {
        const QString relativePath = base.relativeFilePath(fi.absoluteFilePath());
        if (fi.isDir() && matchesSubdirectoryPattern(relativePath)) {
            result += scan(fi.absoluteFilePath(), base);
        } else {
            Core::GeneratedFile f(fi.absoluteFilePath());
            f.setAttributes(f.attributes() | Core::GeneratedFile::KeepExistingFileAttribute);

            result.append(f);
        }
    }

    return result;
}

} // namespace Internal
} // namespace ProjectExplorer
