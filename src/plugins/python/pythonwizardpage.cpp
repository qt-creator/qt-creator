// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonwizardpage.h"

#include "pythonconstants.h"
#include "pythonkitaspect.h"
#include "pythonsettings.h"
#include "pythontr.h"

#include <coreplugin/generatedfile.h>

#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/wizardpage.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

class PythonWizardPage final : public WizardPage
{
public:
    PythonWizardPage(const QList<QPair<QString, QVariant>> &pySideAndData, int defaultPyside);
    bool validatePage() final;

private:
    SelectionAspect m_pySideVersion;
};

class PythonWizardPageFactory final : public JsonWizardPageFactory
{
public:
    PythonWizardPageFactory()
    {
        setTypeIdsSuffix("PythonConfiguration");
    }

    WizardPage *create(JsonWizard *wizard, Id typeId, const QVariant &data) final;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) final;
};

WizardPage *PythonWizardPageFactory::create(JsonWizard *wizard, Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    QList<QPair<QString, QVariant>> pySideAndData;
    for (const QVariant &item : data.toMap().value("items").toList()) {
        const QMap<QString, QVariant> map = item.toMap();
        const QVariant name = map.value("trKey");
        if (name.isValid())
            pySideAndData.emplaceBack(QPair<QString, QVariant>{name.toString(), map.value("value")});
    }
    bool validIndex = false;
    int defaultPySide = data.toMap().value("index").toInt(&validIndex);
    if (!validIndex)
        defaultPySide = -1;
    return new PythonWizardPage(pySideAndData, defaultPySide);
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
            *errorMessage = Tr::tr("\"data\" of a Python wizard page expects a map with \"items\" "
                                   "containing a list of objects.");
        }
        return false;
    }

    if (!Utils::allOf(items, &validItem)) {
        if (errorMessage) {
            *errorMessage = Tr::tr(
                "An item of Python wizard page data expects a \"trKey\" field containing the UI "
                "visible string for that Python version and a \"value\" field containing an object "
                "with a \"PySideVersion\" field used for import statements in the Python files.");
        }
        return false;
    }
    return true;
}

PythonWizardPage::PythonWizardPage(const QList<QPair<QString, QVariant>> &pySideAndData,
                                   const int defaultPyside)
{
    using namespace Layouting;

    m_pySideVersion.setLabelText(Tr::tr("PySide version:"));
    m_pySideVersion.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (auto [name, data] : pySideAndData)
        m_pySideVersion.addOption(SelectionAspect::Option(name, {}, data));
    if (defaultPyside >= 0)
        m_pySideVersion.setDefaultValue(defaultPyside);

    Form {
        m_pySideVersion, st, br,
    }.attachTo(this);
}

bool PythonWizardPage::validatePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    const QMap<QString, QVariant> data = m_pySideVersion.itemValue().toMap();
    for (auto it = data.begin(), end = data.end(); it != end; ++it)
        wiz->setValue(it.key(), it.value());
    return true;
}

void setupPythonWizard()
{
    static PythonWizardPageFactory thePythonWizardPageFactory;
}

} // namespace Python::Internal

