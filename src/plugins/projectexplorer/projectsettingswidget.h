/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
    Utils::Id globalSettingsId() const;

protected:
    void setUseGlobalSettingsCheckBoxVisible(bool visible);
    void setGlobalSettingsId(Utils::Id globalId);

signals:
    void useGlobalSettingsChanged(bool useGlobalSettings);
    void useGlobalSettingsCheckBoxEnabledChanged(bool enadled);

private:
    bool m_useGlobalSettings = true;
    bool m_useGlobalSettingsCheckBoxEnabled = true;
    bool m_useGlobalSettingsCheckBoxVisibleVisible = true;
    Utils::Id m_globalSettingsId;
};

} // namespace ProjectExplorer
