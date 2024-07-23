// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizardpagefactory.h"

#include "../projectexplorerconstants.h"

#include <utils/algorithm.h>

namespace ProjectExplorer {

void JsonWizardPageFactory::setTypeIdsSuffixes(const QStringList &suffixes)
{
    m_typeIds = Utils::transform(suffixes, [](const QString &suffix) {
        return Utils::Id(Constants::PAGE_ID_PREFIX).withSuffix(suffix);
    });
}

void JsonWizardPageFactory::setTypeIdsSuffix(const QString &suffix)
{
    setTypeIdsSuffixes({suffix});
}

} // namespace ProjectExplorer
