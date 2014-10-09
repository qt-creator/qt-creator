/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "jsonwizardpagefactory_p.h"

#include "jsonfieldpage.h"
#include "jsonfilepage.h"
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
}

Utils::WizardPage *FieldPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);

    QTC_ASSERT(canCreate(typeId), return 0);

    JsonFieldPage *page = new JsonFieldPage(wizard->expander());

    if (!page->setup(data)) {
        delete page;
        return 0;
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
    Q_UNUSED(data);
    QTC_ASSERT(canCreate(typeId), return 0);

    JsonFilePage *page = new JsonFilePage;
    page->setPath(wizard->value(QStringLiteral("InitialPath")).toString());
    return page;
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
// SummaryPageFactory:
// --------------------------------------------------------------------

SummaryPageFactory::SummaryPageFactory()
{
    setTypeIdsSuffix(QLatin1String("Summary"));
}

Utils::WizardPage *SummaryPageFactory::create(JsonWizard *wizard, Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard);
    Q_UNUSED(data);
    QTC_ASSERT(canCreate(typeId), return 0);

    return new JsonSummaryPage;
}

bool SummaryPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" for a \"Summary\" page needs to be unset or an empty object.");
        return false;
    }

    return true;
}

} // namespace Internal
} // namespace ProjectExplorer
