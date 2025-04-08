// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizardscannergenerator.h"

#include "jsonwizardgeneratorfactory.h"
#include "../projectmanager.h"
#include "../projectexplorertr.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QVariant>

#include <limits>

using namespace Utils;

namespace ProjectExplorer::Internal {

class JsonWizardScannerGenerator final : public JsonWizardGenerator
{
public:
    Result<> setup(const QVariant &data);

    Core::GeneratedFiles fileList(MacroExpander *expander,
                                  const FilePath &wizardDir,
                                  const FilePath &projectDir,
                                  QString *errorMessage) final;
private:
    Core::GeneratedFiles scan(const FilePath &dir, const FilePath &base);
    bool matchesSubdirectoryPattern(const FilePath &path);

    QString m_binaryPattern;
    QList<QRegularExpression> m_subDirectoryExpressions;
};

Result<> JsonWizardScannerGenerator::setup(const QVariant &data)
{
    if (data.isNull())
        return ResultOk;

    if (data.typeId() != QMetaType::QVariantMap)
        return ResultError(Tr::tr("Key is not an object."));

    QVariantMap gen = data.toMap();

    m_binaryPattern = gen.value(QLatin1String("binaryPattern")).toString();
    const QStringList patterns = gen.value(QLatin1String("subdirectoryPatterns")).toStringList();
    for (const QString &pattern : patterns) {
        QRegularExpression regexp(pattern);
        if (!regexp.isValid())
            return ResultError(Tr::tr("Pattern \"%1\" is no valid regular expression."));
        m_subDirectoryExpressions << regexp;
    }

    return ResultOk;
}

Core::GeneratedFiles JsonWizardScannerGenerator::fileList(Utils::MacroExpander *expander,
                                                          const Utils::FilePath &wizardDir,
                                                          const Utils::FilePath &projectDir,
                                                          QString *errorMessage)
{
    Q_UNUSED(wizardDir)
    errorMessage->clear();

    Core::GeneratedFiles result;

    QRegularExpression binaryPattern;
    if (!m_binaryPattern.isEmpty()) {
        binaryPattern = QRegularExpression(expander->expand(m_binaryPattern));
        if (!binaryPattern.isValid()) {
            qWarning() << Tr::tr("ScannerGenerator: Binary pattern \"%1\" not valid.")
                          .arg(m_binaryPattern);
            return result;
        }
    }

    result = scan(projectDir, projectDir);

    static const auto getDepth =
            [](const Utils::FilePath &filePath) { return int(filePath.path().count('/')); };
    int minDepth = std::numeric_limits<int>::max();
    for (auto it = result.begin(); it != result.end(); ++it) {
        const Utils::FilePath relPath = it->filePath().relativePathFromDir(projectDir);
        it->setBinary(binaryPattern.match(relPath.toUrlishString()).hasMatch());
        bool found = ProjectManager::canOpenProjectForMimeType(Utils::mimeTypeForFile(relPath));
        if (found) {
            it->setAttributes(it->attributes() | Core::GeneratedFile::OpenProjectAttribute);
            minDepth = std::min(minDepth, getDepth(it->filePath()));
        }
    }

    // Project files that appear on a lower level in the file system hierarchy than
    // other project files are not candidates for opening.
    for (Core::GeneratedFile &f : result) {
        if (f.attributes().testFlag(Core::GeneratedFile::OpenProjectAttribute)
                && getDepth(f.filePath()) > minDepth) {
            f.setAttributes(f.attributes().setFlag(Core::GeneratedFile::OpenProjectAttribute,
                                                   false));
        }
    }

    return result;
}

bool JsonWizardScannerGenerator::matchesSubdirectoryPattern(const Utils::FilePath &path)
{
    for (const QRegularExpression &regexp : std::as_const(m_subDirectoryExpressions)) {
        if (regexp.match(path.path()).hasMatch())
            return true;
    }
    return false;
}

Core::GeneratedFiles JsonWizardScannerGenerator::scan(const Utils::FilePath &dir,
                                                      const Utils::FilePath &base)
{
    Core::GeneratedFiles result;

    if (!dir.exists())
        return result;

    const Utils::FilePaths entries = dir.dirEntries({{}, QDir::AllEntries | QDir::NoDotAndDotDot},
                                                    QDir::DirsLast | QDir::Name);
    for (const Utils::FilePath &fi : entries) {
        const Utils::FilePath relativePath = fi.relativePathFromDir(base);
        if (fi.isDir() && matchesSubdirectoryPattern(relativePath)) {
            result += scan(fi, base);
        } else {
            Core::GeneratedFile f(fi);
            f.setAttributes(f.attributes() | Core::GeneratedFile::KeepExistingFileAttribute);

            result.append(f);
        }
    }

    return result;
}

void setupJsonWizardScannerGenerator()
{
    static JsonWizardGeneratorTypedFactory<JsonWizardScannerGenerator>
        theScannerGeneratorFactory("Scanner");
}

} // ProjectExplorer::Internal
