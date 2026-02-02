// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprojectwizardpage.h"

#include "projectexplorer/target.h"
#include "qmlproject.h"
#include "qmlprojectmanagertr.h"
#include "utils/fileutils.h"
#include "utils/templateengine.h"

#include <projectexplorer/projectmanager.h>

namespace QmlProjectManager {

using namespace Qt::Literals::StringLiterals;

QmlModuleWizardFieldPage::QmlModuleWizardFieldPage(Utils::MacroExpander *expander, QWidget *parent)
    : JsonFieldPage(expander, parent)
{ }

void QmlModuleWizardFieldPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    if (!wiz)
        return;

    JsonFieldPage::initializePage();
    if (auto* project = qobject_cast<QmlProject *>(ProjectManager::startupProject())) {
        wiz->setProperty("TargetPath", project->projectDirectory().toFSPathString());
        wiz->setProperty("MCU", project->isMCUs());
        wiz->setProperty("Mock", false);
    }
}


QmlModuleWizardFieldPageFactory::QmlModuleWizardFieldPageFactory()
{
    setTypeIdsSuffix("QmlProjectFields"_L1);
}

WizardPage *QmlModuleWizardFieldPageFactory::create(
    JsonWizard *wizard, Id typeId, const QVariant &data)
{
    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = std::make_unique<QmlModuleWizardFieldPage>(wizard->expander());
    if (!page->setup(data))
        return nullptr;

    return page.release();
}

Result<> QmlModuleWizardFieldPageFactory::validateData(Id typeId, const QVariant &data)
{
    QTC_ASSERT(canCreate(typeId), return {});

    const Result<QVariantList> list = JsonWizardFactory::objectOrList(data);
    if (!list) {
        return ResultError(Tr::tr("When parsing fields of page \"%1\": %2")
                               .arg(typeId.toString()).arg(list.error()));
    }
    if (!list->size()) {
        return ResultError(Tr::tr("When parsing fields of page \"%1\": %2")
                               .arg(typeId.toString())
                               .arg(Tr::tr("No entries found.")));
    }

    for (const QVariant &v : *list) {
        Result<JsonFieldPage::Field *> field = JsonFieldPage::Field::parse(v);
        if (!field)
            return ResultError(field.error());
    }

    return ResultOk;
}

Result<> QmlModuleWizardGenerator::setup(const QVariant &data)
{
    QString errorMessage;
    const Result<QVariantList> list = JsonWizardFactory::objectOrList(data);
    if (list->isEmpty())
        return ResultError(list.error());

    const QVariantMap map = list->first().toMap();
    m_source = FilePath::fromSettings(map.value("source"_L1));
    m_target = FilePath::fromSettings(map.value("target"_L1));
    return ResultOk;
}

Core::GeneratedFiles QmlModuleWizardGenerator::fileList(Utils::MacroExpander *expander,
                                                        const Utils::FilePath &wizardDir,
                                                        const Utils::FilePath &projectDir,
                                                        QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();
    auto fail = [&](const QString &m) {
        if (errorMessage)
            *errorMessage = m;
        return Core::GeneratedFiles{};
    };

    const Utils::FilePath sourcePath = wizardDir.resolvePath(expander->expand(m_source));
    if (!sourcePath.exists())
        return fail(Tr::tr("Source template not found: %1").arg(sourcePath.toUserOutput()));

    const Utils::FilePath targetPath = expander->expand(m_target);
    if (!targetPath.isChildOf(projectDir))
        return fail(Tr::tr("Target path is outside the project: %1").arg(targetPath.toUserOutput()));

    const auto fileReadResult = sourcePath.fileContents();
    if (!fileReadResult)
        return fail(fileReadResult.error());

    const auto processedResult = Utils::TemplateEngine::processText(expander,
                                                                    QString::fromUtf8(*fileReadResult));
    if (!processedResult) {
        return fail(Tr::tr("When processing \"%1\":").arg(sourcePath.toUserOutput()) + "<br>"
                    + processedResult.error());
    }

    Core::GeneratedFile file(targetPath);
    file.setContents(*processedResult);
    return {file};
}

Utils::Result<> QmlModuleWizardGenerator::writeFile(const JsonWizard *wizard, Core::GeneratedFile *file)
{
    if (wizard->stringValue("HasComponent") == "false")
        file->setAttributes(file->attributes() | Core::GeneratedFile::OpenEditorAttribute);

    return file->write();
}

Utils::Result<> QmlModuleWizardGenerator::allDone(const JsonWizard * /*wizard*/, GeneratedFile *file)
{
    if (auto* project = qobject_cast<QmlProject*>(ProjectManager::startupProject())) {
        Target *target = project->activeTarget();
        if (!target)
            return ResultError("No active target.");

        auto bs = qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
        if (!bs)
            return ResultError("No qml build system.");

        if (!bs->importPaths().contains("."))
            bs->addImportPath(".");

        Utils::FilePath root = project->rootProjectDirectory();
        Utils::FilePath modulePath = file->filePath().parentDir().relativeChildPath(root);

        if (project->isMCUs()) {
            auto projectFileName = modulePath.baseName() + ".qmlproject";
            auto moduleProjectFile = modulePath.pathAppended(projectFileName);
            bs->addQmlProjectModule(moduleProjectFile);
        } else {
            bs->addFileFilter(modulePath);
        }
        return ResultOk;
    }
    return ResultError("No startup project.");
}

void setupQmlModuleWizard()
{
    static QmlModuleWizardFieldPageFactory theFieldPageFactory;

    static JsonWizardGeneratorTypedFactory<QmlModuleWizardGenerator>
        theQmlProjectGeneratorFactory("QmlProjectFile");
}

} // namespace QmlProjectManager
