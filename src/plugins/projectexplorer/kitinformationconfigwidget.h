/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITINFORMATIONCONFIGWIDGET_H
#define KITINFORMATIONCONFIGWIDGET_H

#include "kitconfigwidget.h"

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
class ToolChain;

namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

class SysRootInformationConfigWidget : public KitConfigWidget
{
    Q_OBJECT

public:
    SysRootInformationConfigWidget(Kit *k, const KitInformation *ki);
    ~SysRootInformationConfigWidget();

    QString displayName() const;
    void refresh();
    void makeReadOnly();
    QWidget *buttonWidget() const;
    QWidget *mainWidget() const;
    QString toolTip() const;

    void setPalette(const QPalette &p);

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
    ~ToolChainInformationConfigWidget();

    QString displayName() const;
    void refresh();
    void makeReadOnly();
    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;
    QString toolTip() const;

private:
    void manageToolChains();
    void currentToolChainChanged(int idx);

    int indexOf(const ToolChain *tc);

    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
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
    ~DeviceTypeInformationConfigWidget();

    QWidget *mainWidget() const;
    QString displayName() const;
    QString toolTip() const;
    void refresh();
    void makeReadOnly();

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
    ~DeviceInformationConfigWidget();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;
    QString displayName() const;
    QString toolTip() const;
    void refresh();
    void makeReadOnly();

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

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;
    QString displayName() const;
    QString toolTip() const;
    void refresh();
    void makeReadOnly();

private:
    void editEnvironmentChanges();

    void applyChanges();
    void closeChangesDialog();
    void acceptChangesDialog();

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QDialog *m_dialog = 0;
    QPlainTextEdit *m_editor = 0;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITINFORMATIONCONFIGWIDGET_H
