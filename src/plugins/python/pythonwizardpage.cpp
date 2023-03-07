// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonwizardpage.h"

#include "pythonconstants.h"
#include "pythonsettings.h"
#include "pythontr.h"

#include <coreplugin/generatedfile.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

PythonWizardPageFactory::PythonWizardPageFactory()
{
    setTypeIdsSuffix("PythonConfiguration");
}

WizardPage *PythonWizardPageFactory::create(JsonWizard *wizard, Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = new PythonWizardPage;
    for (const QVariant &item : data.toMap().value("items").toList()) {
        const QMap<QString, QVariant> map = item.toMap();
        const QVariant name = map.value("trKey");
        if (name.isValid())
            page->addPySideVersions(name.toString(), map.value("value"));
    }
    bool validIndex = false;
    const int index = data.toMap().value("index").toInt(&validIndex);
    if (validIndex)
        page->setDefaultPySideVersions(index);
    return page;
}

static bool validItem(const QVariant &item)
{
    QMap<QString, QVariant> map = item.toMap();
    if (!map.value("trKey").canConvert<QString>())
        return false;
    map = map.value("value").toMap();
    return map.value("PySideVersion").canConvert<QString>();
}

bool PythonWizardPageFactory::validateData(Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    const QList<QVariant> items = data.toMap().value("items").toList();

    if (items.isEmpty()) {
        if (errorMessage) {
            *errorMessage = Tr::tr("'data' of a Python wizard page expects a map with 'items' "
                                   "containing a list of objects");
        }
        return false;
    }

    if (!Utils::allOf(items, &validItem)) {
        if (errorMessage) {
            *errorMessage = Tr::tr(
                "An item of Python wizard page data expects a 'trKey' field containing the ui "
                "visible string for that python version and an field 'value' containing an object "
                "with a 'PySideVersion' field used for import statements in the python files.");
        }
        return false;
    }
    return true;


    if (!items.isEmpty() && Utils::allOf(items, &validItem))
        return true;

}

PythonWizardPage::PythonWizardPage()
{
    m_interpreter.setSettingsDialogId(Constants::C_PYTHONOPTIONS_PAGE_ID);
    connect(PythonSettings::instance(),
            &PythonSettings::interpretersChanged,
            this,
            &PythonWizardPage::updateInterpreters);

    m_pySideVersion.setLabelText(Tr::tr("PySide version"));
    m_pySideVersion.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
}

void PythonWizardPage::initializePage()
{
    using namespace Utils::Layouting;

    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    updateInterpreters();

    connect(wiz, &JsonWizard::filesPolished, this, &PythonWizardPage::setupProject);

    Grid {
         m_pySideVersion, br,
         m_interpreter, br
    }.attachTo(this, WithoutMargins);
}

bool PythonWizardPage::validatePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    const QMap<QString, QVariant> data = m_pySideVersion.itemValue().toMap();
    for (auto it = data.begin(), end = data.end(); it != end; ++it)
        wiz->setValue(it.key(), it.value());
    return true;
}

void PythonWizardPage::addPySideVersions(const QString &name, const QVariant &data)
{
    m_pySideVersion.addOption(SelectionAspect::Option(name, {}, data));
}

void PythonWizardPage::setDefaultPySideVersions(int index)
{
    m_pySideVersion.setDefaultValue(index);
}

void PythonWizardPage::setupProject(const JsonWizard::GeneratorFiles &files)
{
    for (const JsonWizard::GeneratorFile &f : files) {
        if (f.file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            Project *project = ProjectManager::openProject(Utils::mimeTypeForFile(f.file.filePath()),
                                                           f.file.filePath().absoluteFilePath());
            if (project) {
                project->addTargetForDefaultKit();
                if (Target *target = project->activeTarget()) {
                    if (RunConfiguration *rc = target->activeRunConfiguration()) {
                        if (auto interpreters = rc->aspect<InterpreterAspect>()) {
                            interpreters->setCurrentInterpreter(m_interpreter.currentInterpreter());
                            project->saveSettings();
                        }
                    }
                }
                delete project;
            }
        }
    }
}

void PythonWizardPage::updateInterpreters()
{
    m_interpreter.setDefaultInterpreter(PythonSettings::defaultInterpreter());
    m_interpreter.updateInterpreters(PythonSettings::interpreters());
}

} // namespace Python::Internal

