/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILEINFORMATIONCONFIGWIDGET_H
#define PROFILEINFORMATIONCONFIGWIDGET_H

#include "profileconfigwidget.h"

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Utils { class PathChooser; }

namespace ProjectExplorer {

class DeviceManagerModel;
class Profile;
class ToolChain;

namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

class SysRootInformationConfigWidget : public ProfileConfigWidget
{
    Q_OBJECT

public:
    explicit SysRootInformationConfigWidget(Profile *p, QWidget *parent = 0);

    QString displayName() const;
    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

private:
    Profile *m_profile;
    Utils::PathChooser *m_chooser;
};

// --------------------------------------------------------------------------
// ToolChainInformationConfigWidget:
// --------------------------------------------------------------------------

class ToolChainInformationConfigWidget : public ProfileConfigWidget
{
    Q_OBJECT

public:
    explicit ToolChainInformationConfigWidget(Profile *p, QWidget *parent = 0);

    QString displayName() const;
    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

private slots:
    void toolChainAdded(ProjectExplorer::ToolChain *tc);
    void toolChainRemoved(ProjectExplorer::ToolChain *tc);
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void manageToolChains();

private:
    void updateComboBox();
    int indexOf(const ToolChain *tc);

    bool m_isReadOnly;
    Profile *m_profile;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

// --------------------------------------------------------------------------
// DeviceTypeInformationConfigWidget:
// --------------------------------------------------------------------------

class DeviceTypeInformationConfigWidget : public ProfileConfigWidget
{
    Q_OBJECT

public:
    explicit DeviceTypeInformationConfigWidget(Profile *p, QWidget *parent = 0);

    QString displayName() const;
    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

private:
    bool m_isReadOnly;
    Profile *m_profile;
    QComboBox *m_comboBox;
};

// --------------------------------------------------------------------------
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

class DeviceInformationConfigWidget : public ProfileConfigWidget
{
    Q_OBJECT

public:
    explicit DeviceInformationConfigWidget(Profile *p, QWidget *parent = 0);

    QString displayName() const;
    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

private slots:
    void manageDevices();

private:
    bool m_isReadOnly;
    Profile *m_profile;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
    DeviceManagerModel *m_model;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROFILEINFORMATIONCONFIGWIDGET_H
