// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

#include <QWidget>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT ProjectSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectSettingsWidget(QWidget *parent = nullptr);

    void setUseGlobalSettings(bool useGlobalSettings);
    bool useGlobalSettings() const;

    void setUseGlobalSettingsCheckBoxEnabled(bool enadled);
    bool isUseGlobalSettingsCheckBoxEnabled() const;

    bool isUseGlobalSettingsCheckBoxVisible() const;
    bool isUseGlobalSettingsLabelVisible() const;
    Utils::Id globalSettingsId() const;

    bool expanding() const;
    void setExpanding(bool expanding);

protected:
    void setUseGlobalSettingsCheckBoxVisible(bool visible);
    void setUseGlobalSettingsLabelVisible(bool visible);
    void setGlobalSettingsId(Utils::Id globalId);

signals:
    void useGlobalSettingsChanged(bool useGlobalSettings);
    void useGlobalSettingsCheckBoxEnabledChanged(bool enadled);

private:
    bool m_useGlobalSettings = true;
    bool m_useGlobalSettingsCheckBoxEnabled = true;
    bool m_useGlobalSettingsCheckBoxVisibleVisible = true;
    bool m_useGlobalSettingsLabelVisibleVisible = true;
    bool m_expanding = false;
    Utils::Id m_globalSettingsId;
};

} // namespace ProjectExplorer
