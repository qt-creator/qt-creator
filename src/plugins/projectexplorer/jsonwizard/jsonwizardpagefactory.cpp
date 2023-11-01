// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizardpagefactory.h"

#include "../projectexplorerconstants.h"

#include <utils/algorithm.h>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// JsonWizardPageFactory:
// --------------------------------------------------------------------

void JsonWizardPageFactory::setTypeIdsSuffixes(const QStringList &suffixes)
{
    m_typeIds = Utils::transform(suffixes, [](const QString &suffix) {
        return Utils::Id::fromString(QString::fromLatin1(Constants::PAGE_ID_PREFIX) + suffix);});
}

void JsonWizardPageFactory::setTypeIdsSuffix(const QString &suffix)
{
    setTypeIdsSuffixes(QStringList() << suffix);
}

} // namespace ProjectExplorer
