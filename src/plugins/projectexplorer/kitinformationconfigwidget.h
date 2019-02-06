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

#pragma once

#include "kitconfigwidget.h"
#include "toolchain.h"

#include <coreplugin/id.h>

#include <utils/environment.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {

class DeviceManagerModel;

namespace Internal {

// --------------------------------------------------------------------------
// SysRootKitAspectWidget:
// --------------------------------------------------------------------------

class SysRootKitAspectWidget : public KitAspectWidget
{
    Q_OBJECT

public:
    SysRootKitAspectWidget(Kit *k, const KitAspect *ki);
    ~SysRootKitAspectWidget() override;

    QString displayName() const override;
    void refresh() override;
    void makeReadOnly() override;
    QWidget *buttonWidget() const override;
    QWidget *mainWidget() const override;
    QString toolTip() const override;

    void setPalette(const QPalette &p) override;

private:
    void pathWasChanged();

    Utils::PathChooser *m_chooser;
    bool m_ignoreChange = false;
};

// --------------------------------------------------------------------------
// ToolChainKitAspectWidget:
// --------------------------------------------------------------------------

class ToolChainKitAspectWidget : public KitAspectWidget
{
    Q_OBJECT

public:
    ToolChainKitAspectWidget(Kit *k, const KitAspect *ki);
    ~ToolChainKitAspectWidget() override;

    QString displayName() const override;
    void refresh() override;
    void makeReadOnly() override;
    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString toolTip() const override;

private:
    void manageToolChains();
    void currentToolChainChanged(Core::Id language, int idx);

    int indexOf(QComboBox *cb, const ToolChain *tc);

    QWidget *m_mainWidget = nullptr;
    QPushButton *m_manageButton = nullptr;
    QHash<Core::Id, QComboBox *> m_languageComboboxMap;
    bool m_ignoreChanges = false;
    bool m_isReadOnly = false;
};

// --------------------------------------------------------------------------
// DeviceTypeKitAspectWidget:
// --------------------------------------------------------------------------

class DeviceTypeKitAspectWidget : public KitAspectWidget
{
    Q_OBJECT

public:
    DeviceTypeKitAspectWidget(Kit *workingCopy, const KitAspect *ki);
    ~DeviceTypeKitAspectWidget() override;

    QWidget *mainWidget() const override;
    QString displayName() const override;
    QString toolTip() const override;
    void refresh() override;
    void makeReadOnly() override;

private:
    void currentTypeChanged(int idx);

    QComboBox *m_comboBox;
};

// --------------------------------------------------------------------------
// DeviceKitAspectWidget:
// --------------------------------------------------------------------------

class DeviceKitAspectWidget : public KitAspectWidget
{
    Q_OBJECT

public:
    DeviceKitAspectWidget(Kit *workingCopy, const KitAspect *ki);
    ~DeviceKitAspectWidget() override;

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString displayName() const override;
    QString toolTip() const override;
    void refresh() override;
    void makeReadOnly() override;

private:
    void manageDevices();
    void modelAboutToReset();
    void modelReset();
    void currentDeviceChanged();

    bool m_isReadOnly = false;
    bool m_ignoreChange = false;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
    DeviceManagerModel *m_model;
    Core::Id m_selectedId;
};

class EnvironmentKitAspectWidget : public KitAspectWidget
{
    Q_OBJECT

public:
    EnvironmentKitAspectWidget(Kit *workingCopy, const KitAspect *ki);

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString displayName() const override;
    QString toolTip() const override;
    void refresh() override;
    void makeReadOnly() override;

private:
    void editEnvironmentChanges();
    QList<Utils::EnvironmentItem> currentEnvironment() const;

    void initMSVCOutputSwitch(QVBoxLayout *layout);

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QCheckBox *m_vslangCheckbox;
    QWidget *m_mainWidget;
};

} // namespace Internal
} // namespace ProjectExplorer
