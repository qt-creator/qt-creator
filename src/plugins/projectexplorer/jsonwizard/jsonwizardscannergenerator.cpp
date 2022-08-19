// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "jsonwizardscannergenerator.h"

#include "../projectmanager.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QVariant>

#include <limits>

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
    const QStringList patterns = gen.value(QLatin1String("subdirectoryPatterns")).toStringList();
    for (const QString &pattern : patterns) {
        QRegularExpression regexp(pattern);
        if (!regexp.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::Internal::JsonWizard",
                                                        "Pattern \"%1\" is no valid regular expression.");
            return false;
        }
        m_subDirectoryExpressions << regexp;
    }

    return true;
}

Core::GeneratedFiles JsonWizardScannerGenerator::fileList(Utils::MacroExpander *expander,
                                                          const QString &wizardDir,
                                                          const QString &projectDir,
                                                          QString *errorMessage)
{
    Q_UNUSED(wizardDir)
    errorMessage->clear();

    QDir project(projectDir);
    Core::GeneratedFiles result;

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

    result = scan(project.absolutePath(), project);

    static const auto getDepth = [](const QString &filePath) { return int(filePath.count('/')); };
    int minDepth = std::numeric_limits<int>::max();
    for (auto it = result.begin(); it != result.end(); ++it) {
        const QString relPath = project.relativeFilePath(it->path());
        it->setBinary(binaryPattern.match(relPath).hasMatch());
        bool found = ProjectManager::canOpenProjectForMimeType(Utils::mimeTypeForFile(
                              Utils::FilePath::fromString(relPath)));
        if (found) {
            it->setAttributes(it->attributes() | Core::GeneratedFile::OpenProjectAttribute);
            minDepth = std::min(minDepth, getDepth(it->path()));
        }
    }

    // Project files that appear on a lower level in the file system hierarchy than
    // other project files are not candidates for opening.
    for (Core::GeneratedFile &f : result) {
        if (f.attributes().testFlag(Core::GeneratedFile::OpenProjectAttribute)
                && getDepth(f.path()) > minDepth) {
            f.setAttributes(f.attributes().setFlag(Core::GeneratedFile::OpenProjectAttribute,
                                                   false));
        }
    }

    return result;
}

bool JsonWizardScannerGenerator::matchesSubdirectoryPattern(const QString &path)
{
    for (const QRegularExpression &regexp : qAsConst(m_subDirectoryExpressions)) {
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

    const QFileInfoList entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                          QDir::DirsLast | QDir::Name);
    for (const QFileInfo &fi : entries) {
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
