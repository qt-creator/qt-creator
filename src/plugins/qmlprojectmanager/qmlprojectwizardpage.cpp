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

bool QmlModuleWizardFieldPageFactory::validateData(
    Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    const QList<QVariant> list = JsonWizardFactory::objectOrList(data, errorMessage);
    if (list.isEmpty()) {
        *errorMessage = Tr::tr("When parsing fields of page \"%1\": %2")
                .arg(typeId.toString()).arg(*errorMessage);
        return false;
    }

    for (const QVariant &v : list) {
        std::unique_ptr<JsonFieldPage::Field> field(JsonFieldPage::Field::parse(v, errorMessage));
        if (!field)
            return false;
    }

    return true;
}


bool QmlModuleWizardGenerator::setup(const QVariant &data, QString *errorMessage)
{
    const QVariantList list = JsonWizardFactory::objectOrList(data, errorMessage);
    if (list.isEmpty())
        return false;

    const QVariantMap map = list.first().toMap();
    m_source = FilePath::fromSettings(map.value("source"_L1));
    m_target = FilePath::fromSettings(map.value("target"_L1));
    return true;
}

Core::GeneratedFiles QmlModuleWizardGenerator::fileList(
        Utils::MacroExpander *expander,
        const Utils::FilePath &wizardDir,
        const Utils::FilePath &projectDir,
        QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();

    Utils::FilePath source = wizardDir.resolvePath(expander->expand(m_source));
    if (!source.exists())
        return {};

    Utils::FilePath target = expander->expand(m_target);
    if (!target.isChildOf(projectDir))
        return {};

    FileReader reader;
    if (!reader.fetch(source, errorMessage))
        return {};

    GeneratedFile file;
    file.setFilePath(target);
    file.setContents(Utils::TemplateEngine::processText(
            expander,
            QString::fromUtf8(reader.text()),
            errorMessage));

    if (!errorMessage->isEmpty()) {
        *errorMessage = Tr::tr("When processing \"%1\":<br>%2")
                .arg(source.toUserOutput(), *errorMessage);
        return { };
    }

    return {file};
}

bool QmlModuleWizardGenerator::writeFile(
    const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage)
{
    if (wizard->stringValue("HasComponent") == "false")
        file->setAttributes(file->attributes() | Core::GeneratedFile::OpenEditorAttribute);

    if (!file->write(errorMessage))
        return false;

    return true;
}

bool QmlModuleWizardGenerator::allDone(
    const JsonWizard * /*wizard*/, GeneratedFile *file, QString * /*errorMessage*/)
{
    if (auto* project = qobject_cast<QmlProject*>(ProjectManager::startupProject())) {
        Target *target = project->activeTarget();
        if (!target)
            return false;

        auto bs = qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
        if (!bs)
            return false;


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
        return true;
    }
    return false;
}


void setupQmlModuleWizard()
{
    static QmlModuleWizardFieldPageFactory theFieldPageFactory;

    static JsonWizardGeneratorTypedFactory<QmlModuleWizardGenerator>
        theQmlProjectGeneratorFactory("QmlProjectFile");
}

} // namespace QmlProjectManager
