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
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {

class DeviceManagerModel;

namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

class SysRootInformationConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    SysRootInformationConfigWidget(Kit *k, const KitInformation *ki);
    ~SysRootInformationConfigWidget() override;

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
// ToolChainInformationConfigWidget:
// --------------------------------------------------------------------------

class ToolChainInformationConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    ToolChainInformationConfigWidget(Kit *k, const KitInformation *ki);
    ~ToolChainInformationConfigWidget() override;

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

    QWidget *m_mainWidget;
    QPushButton *m_manageButton;
    QHash<Core::Id, QComboBox *> m_languageComboboxMap;
    bool m_ignoreChanges = false;
    bool m_isReadOnly = false;
};

// --------------------------------------------------------------------------
// DeviceTypeInformationConfigWidget:
// --------------------------------------------------------------------------

class DeviceTypeInformationConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    DeviceTypeInformationConfigWidget(Kit *workingCopy, const KitInformation *ki);
    ~DeviceTypeInformationConfigWidget() override;

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
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

class DeviceInformationConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    DeviceInformationConfigWidget(Kit *workingCopy, const KitInformation *ki);
    ~DeviceInformationConfigWidget() override;

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

class KitEnvironmentConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    KitEnvironmentConfigWidget(Kit *workingCopy, const KitInformation *ki);

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString displayName() const override;
    QString toolTip() const override;
    void refresh() override;
    void makeReadOnly() override;

private:
    void editEnvironmentChanges();
    QList<Utils::EnvironmentItem> currentEnvironment() const;

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace ProjectExplorer
