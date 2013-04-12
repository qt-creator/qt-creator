/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYSETUPWIZARDPAGES_H
#define QNX_INTERNAL_BLACKBERRYSETUPWIZARDPAGES_H

#include <QWizardPage>

namespace Utils {
class PathChooser;
}

namespace Qnx {
namespace Internal {
namespace Ui {

class BlackBerrySetupWizardKeysPage;
class BlackBerrySetupWizardDevicePage;
class BlackBerrySetupWizardFinishPage;

} // namespace Ui

class BlackBerryCsjRegistrar;
class BlackBerryCertificate;
class BlackBerryNDKSettingsWidget;

class BlackBerrySetupWizardWelcomePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerrySetupWizardWelcomePage(QWidget *parent = 0);
};

//-----------------------------------------------------------------------------

class BlackBerrySetupWizardNdkPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerrySetupWizardNdkPage(QWidget *parent = 0);
    virtual ~BlackBerrySetupWizardNdkPage();

    bool isComplete() const;

private:
    BlackBerryNDKSettingsWidget *m_widget;
};

//-----------------------------------------------------------------------------

class BlackBerrySetupWizardKeysPage : public QWizardPage
{
    Q_OBJECT
public:
    static const char PbdtPathField[];
    static const char RdkPathField[];
    static const char CsjPinField[];
    static const char PasswordField[];
    static const char Password2Field[];

    explicit BlackBerrySetupWizardKeysPage(QWidget *parent = 0);
    virtual ~BlackBerrySetupWizardKeysPage();

    bool isComplete() const;

private slots:
    void csjAutoComplete(const QString &path);
    void validateFields();
    void showKeysMessage(const QString &url);

private:
    void initUi();
    void setupCsjPathChooser(Utils::PathChooser *chooser);
    void setComplete(bool complete);

    Ui::BlackBerrySetupWizardKeysPage *m_ui;
    bool m_complete;
};

//-----------------------------------------------------------------------------

class BlackBerrySetupWizardDevicePage : public QWizardPage
{
    Q_OBJECT
public:
    static const char NameField[];
    static const char IpAddressField[];
    static const char PasswordField[];
    static const char PhysicalDeviceField[];

    explicit BlackBerrySetupWizardDevicePage(QWidget *parent = 0);

    bool isComplete() const;

private:
    Ui::BlackBerrySetupWizardDevicePage *m_ui;
};

//-----------------------------------------------------------------------------

class BlackBerrySetupWizardFinishPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerrySetupWizardFinishPage(QWidget *parent = 0);

    void setProgress(const QString &status, int progress);

private:
    Ui::BlackBerrySetupWizardFinishPage *m_ui;
};

//-----------------------------------------------------------------------------

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYSETUPWIZARDPAGES_H
