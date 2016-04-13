/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "jsonwizardpagefactory_p.h"

#include "jsonfieldpage.h"
#include "jsonfieldpage_p.h"
#include "jsonfilepage.h"
#include "jsonkitspage.h"
#include "jsonprojectpage.h"
#include "jsonsummarypage.h"
#include "jsonwizardfactory.h"

#include <utils/qtcassert.h>
#include <utils/wizardpage.h>

#include <QCoreApplication>
#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------
// FieldPageFactory:
// --------------------------------------------------------------------

FieldPageFactory::FieldPageFactory()
{
    setTypeIdsSuffix(QLatin1String("Fields"));

    JsonFieldPage::registerFieldFactory(QLatin1String("Label"), []() { return new LabelField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("Spacer"), []() { return new SpacerField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("LineEdit"), []() { return new LineEditField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("TextEdit"), []() { return new TextEditField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("PathChooser"), []() { return new PathChooserField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("CheckBox"), []() { return new CheckBoxField; });
    JsonFieldPage::registerFieldFactory(QLatin1String("ComboBox"), []() { return new ComboBoxField; });
}

Utils::WizardPage *FieldPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);

    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = new JsonFieldPage(wizard->expander());

    if (!page->setup(data)) {
        delete page;
        return nullptr;
    }

    return page;
}

bool FieldPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    QList<QVariant> list = JsonWizardFactory::objectOrList(data, errorMessage);
    if (list.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "When parsing fields of page '%1': %2")
                .arg(typeId.toString()).arg(*errorMessage);
        return false;
    }

    foreach (const QVariant &v, list) {
        JsonFieldPage::Field *field = JsonFieldPage::Field::parse(v, errorMessage);
        if (!field)
            return false;
        delete field;
    }

    return true;
}

// --------------------------------------------------------------------
// FilePageFactory:
// --------------------------------------------------------------------

FilePageFactory::FilePageFactory()
{
    setTypeIdsSuffix(QLatin1String("File"));
}

Utils::WizardPage *FilePageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);
    Q_UNUSED(data);
    QTC_ASSERT(canCreate(typeId), return nullptr);

    return new JsonFilePage;
}

bool FilePageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" for a \"File\" page needs to be unset or an empty object.");
        return false;
    }

    return true;
}

// --------------------------------------------------------------------
// KitsPageFactory:
// --------------------------------------------------------------------

static const char KEY_PROJECT_FILE[] = "projectFilePath";
static const char KEY_REQUIRED_FEATURES[] = "requiredFeatures";
static const char KEY_PREFERRED_FEATURES[] = "preferredFeatures";

KitsPageFactory::KitsPageFactory()
{
    setTypeIdsSuffix(QLatin1String("Kits"));
}

Utils::WizardPage *KitsPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);
    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = new JsonKitsPage;
    const QVariantMap dataMap = data.toMap();
    page->setUnexpandedProjectPath(dataMap.value(QLatin1String(KEY_PROJECT_FILE)).toString());
    page->setRequiredFeatures(dataMap.value(QLatin1String(KEY_REQUIRED_FEATURES)));
    page->setPreferredFeatures(dataMap.value(QLatin1String(KEY_PREFERRED_FEATURES)));

    return page;
}

static bool validateFeatureList(const QVariantMap &data, const QByteArray &key, QString *errorMessage)
{
    QString message;
    JsonKitsPage::parseFeatures(data.value(QLatin1String(key)), &message);
    if (!message.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "Error parsing \"%1\" in \"Kits\" page: %2")
                .arg(QLatin1String(key), message);
        return false;
    }
    return true;
}

bool KitsPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    if (data.isNull() || data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" must be a JSON object for \"Kits\" pages.");
        return false;
    }

    QVariantMap tmp = data.toMap();
    if (tmp.value(QLatin1String(KEY_PROJECT_FILE)).toString().isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"Kits\" page requires a \"%1\" set.")
                .arg(QLatin1String(KEY_PROJECT_FILE));
        return false;
    }

    return validateFeatureList(tmp, KEY_REQUIRED_FEATURES, errorMessage)
            && validateFeatureList(tmp, KEY_PREFERRED_FEATURES, errorMessage);
}

// --------------------------------------------------------------------
// ProjectPageFactory:
// --------------------------------------------------------------------

static const char KEY_PROJECT_NAME_VALIDATOR[] = "projectNameValidator";

ProjectPageFactory::ProjectPageFactory()
{
    setTypeIdsSuffix(QLatin1String("Project"));
}

Utils::WizardPage *ProjectPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);
    Q_UNUSED(data);
    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = new JsonProjectPage;

    QVariantMap tmp = data.isNull() ? QVariantMap() : data.toMap();
    QString description
            = tmp.value(QLatin1String("trDescription"), QLatin1String("%{trDescription}")).toString();
    page->setDescription(wizard->expander()->expand(description));
    QString projectNameValidator
            = tmp.value(QLatin1String(KEY_PROJECT_NAME_VALIDATOR)).toString();
    if (!projectNameValidator.isEmpty()) {
        QRegularExpression regularExpression(projectNameValidator);
        if (regularExpression.isValid())
            page->setProjectNameRegularExpression(regularExpression);
    }

    return page;
}

bool ProjectPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" must be empty or a JSON object for \"Project\" pages.");
        return false;
    }
    QVariantMap tmp = data.toMap();
    QString projectNameValidator
        = tmp.value(QLatin1String(KEY_PROJECT_NAME_VALIDATOR)).toString();
    if (!projectNameValidator.isNull()) {
        QRegularExpression regularExpression(projectNameValidator);
        if (!regularExpression.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                "Invalid regular expression \"%1\" in \"%2\". %3").arg(
                projectNameValidator, QLatin1String(KEY_PROJECT_NAME_VALIDATOR), regularExpression.errorString());
            return false;
        }
    }

    return true;
}

// --------------------------------------------------------------------
// SummaryPageFactory:
// --------------------------------------------------------------------

static const char KEY_HIDE_PROJECT_UI[] = "hideProjectUi";

SummaryPageFactory::SummaryPageFactory()
{
    setTypeIdsSuffix(QLatin1String("Summary"));
}

Utils::WizardPage *SummaryPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);
    Q_UNUSED(data);
    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto page = new JsonSummaryPage;
    QVariant hideProjectUi = data.toMap().value(QLatin1String(KEY_HIDE_PROJECT_UI));
    page->setHideProjectUiValue(hideProjectUi);
    return page;
}

bool SummaryPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
            "\"data\" for a \"Summary\" page can be unset or needs to be an object.");
        return false;
    }

    return true;
}

} // namespace Internal
} // namespace ProjectExplorer
