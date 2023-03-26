// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "presetmodel.h"

#include <utils/id.h>

namespace Core {
class IWizardFactory;
}

namespace ProjectExplorer {
class JsonWizardFactory;
}

using ProjectExplorer::JsonWizardFactory;

namespace StudioWelcome {

class WizardFactories
{
public:
    using GetIconUnicodeFunc = QString (*)(const QString &);

public:
    WizardFactories(const QList<Core::IWizardFactory *> &factories, QWidget *wizardParent,
                    const Utils::Id &platform);

    const Core::IWizardFactory *front() const;
    const std::map<QString, WizardCategory> &presetsGroupedByCategory() const
    { return m_presetItems; }

    bool empty() const { return m_factories.empty(); }
    static GetIconUnicodeFunc setIconUnicodeCallback(GetIconUnicodeFunc cb)
    {
        return std::exchange(m_getIconUnicode, cb);
    }

private:
    void sortByCategoryAndId();
    void filter();

    std::shared_ptr<PresetItem> makePresetItem(JsonWizardFactory *f, QWidget *parent, const Utils::Id &platform);
    std::map<QString, WizardCategory> makePresetItemsGroupedByCategory();

private:
    QWidget *m_wizardParent;
    Utils::Id m_platform; // filter wizards to only those supported by this platform.

    QList<JsonWizardFactory *> m_factories;
    std::map<QString, WizardCategory> m_presetItems;

    static GetIconUnicodeFunc m_getIconUnicode;
};

} // namespace StudioWelcome
