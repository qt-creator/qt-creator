// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/id.h>

#include <QVariant>
#include <QStringList>

namespace Utils { class WizardPage; }

namespace ProjectExplorer {
class JsonWizard;

class PROJECTEXPLORER_EXPORT JsonWizardPageFactory
{
public:
    JsonWizardPageFactory();
    virtual ~JsonWizardPageFactory();

    bool canCreate(Utils::Id typeId) const { return m_typeIds.contains(typeId); }
    const QList<Utils::Id> &supportedIds() const { return m_typeIds; }

    virtual Utils::WizardPage *create(JsonWizard *wizard, Utils::Id typeId, const QVariant &data) = 0;

    // Basic syntax check for the data taken from the wizard.json file:
    virtual bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) = 0;

protected:
    // This will add "PE.Wizard.Page." in front of the suffixes and set those as supported typeIds
    void setTypeIdsSuffixes(const QStringList &suffixes);
    void setTypeIdsSuffix(const QString &suffix);

private:
    QList<Utils::Id> m_typeIds;
};

} // namespace ProjectExplorer
