// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "useglobalaspect.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QLabel>

using namespace Utils;

namespace ProjectExplorer {

UseGlobalAspect::UseGlobalAspect(Id settingsPageId, AspectContainer *container)
    : BoolAspect(container)
    , m_settingsPageId(settingsPageId)
{
    setDefaultValue(true);
}

void UseGlobalAspect::setSettingsPageId(Id settingsPageId)
{
    m_settingsPageId = settingsPageId;
}

void UseGlobalAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    using namespace Layouting;
    const auto label = new QLabel(QStringLiteral("Use <a href=\"dummy\">global settings</a>"));
    QObject::connect(label, &QLabel::linkActivated, label, [id = m_settingsPageId] {
        Core::ICore::showSettings(id);
    });
    Row row;
    BoolAspect::addToLayoutImpl(row);
    row.addItem(label);
    st(&row);
    parent.addItem(row);
    hr(&parent);
}

} // namespace ProjectExplorer
