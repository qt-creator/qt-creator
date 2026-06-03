// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

#include <QWidget>

namespace ProjectExplorer {

PROJECTEXPLORER_EXPORT QWidget *createGlobalSettingsLink(Utils::Id globalId);

class PROJECTEXPLORER_EXPORT ProjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    ProjectSettingsWidget() = default;

    void setUseGlobalSettings(bool useGlobalSettings);
    bool useGlobalSettings() const;

    void setUseGlobalSettingsCheckBoxEnabled(bool enadled);

    QWidget *createGlobalOrProjectSelector();

protected:
    void setGlobalSettingsId(Utils::Id globalId);

signals:
    void useGlobalSettingsChanged(bool useGlobalSettings);
    void useGlobalSettingsCheckBoxEnabledChanged(bool enadled);

private:
    bool m_useGlobalSettings = true;
    bool m_useGlobalSettingsCheckBoxEnabled = true;
    Utils::Id m_globalSettingsId;
};

} // namespace ProjectExplorer
