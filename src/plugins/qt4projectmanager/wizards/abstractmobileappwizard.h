/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef ABSTRACTMOBILEAPPWIZARD_H
#define ABSTRACTMOBILEAPPWIZARD_H

#include <qt4projectmanager/qt4projectmanager_global.h>
#include <qtsupport/qtversionmanager.h>
#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>
#include <qt4projectmanager/wizards/abstractmobileapp.h>

namespace Qt4ProjectManager {

class AbstractMobileApp;
class TargetSetupPage;

namespace Internal {
class MobileAppWizardGenericOptionsPage;
class MobileAppWizardMaemoOptionsPage;
class MobileAppWizardHarmattanOptionsPage;
}

/// \internal
class QT4PROJECTMANAGER_EXPORT AbstractMobileAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT

protected:
    explicit AbstractMobileAppWizardDialog(QWidget *parent, const QtSupport::QtVersionNumber &minimumQtVersionNumber,
                                           const QtSupport::QtVersionNumber &maximumQtVersionNumber,
                                           const Core::WizardDialogParameters &parameters);
    void addMobilePages();

public:
    TargetSetupPage *targetsPage() const;

protected:
    int addPageWithTitle(QWizardPage *page, const QString &title);
    virtual void initializePage(int id);
    virtual void setIgnoreGenericOptionsPage(bool);
    virtual int nextId() const;

    Utils::WizardProgressItem *targetsPageItem() const;

private:
    int idOfNextGenericPage() const;
    Utils::WizardProgressItem *itemOfNextGenericPage() const;
    bool isQtPlatformSelected(const QString &platform) const;
    QList<Core::Id> selectedKits() const;

    Internal::MobileAppWizardGenericOptionsPage *m_genericOptionsPage;
    Internal::MobileAppWizardMaemoOptionsPage *m_maemoOptionsPage;
    Internal::MobileAppWizardHarmattanOptionsPage *m_harmattanOptionsPage;
    TargetSetupPage *m_targetsPage;

    int m_genericOptionsPageId;
    int m_maemoOptionsPageId;
    int m_harmattanOptionsPageId;
    int m_targetsPageId;
    bool m_ignoreGeneralOptions; // If true, do not show generic mobile options page.
    Utils::WizardProgressItem *m_targetItem;
    Utils::WizardProgressItem *m_genericItem;
    Utils::WizardProgressItem *m_maemoItem;
    Utils::WizardProgressItem *m_harmattanItem;
    QList<Core::Id> m_kitIds;

    friend class AbstractMobileAppWizard;
};

/// \internal
class QT4PROJECTMANAGER_EXPORT AbstractMobileAppWizard : public Core::BaseFileWizard
{
    Q_OBJECT
protected:
    explicit AbstractMobileAppWizard(const Core::BaseFileWizardParameters &params,
        QObject *parent = 0);

private slots:
    void useProjectPath(const QString &projectName, const QString &projectPath);

protected:
    virtual QString fileToOpenPostGeneration() const = 0;

private:
    virtual QWizard *createWizardDialog(QWidget *parent,
        const Core::WizardDialogParameters &wizardDialogParameters) const;
    virtual Core::GeneratedFiles generateFiles(const QWizard *wizard,
        QString *errorMessage) const;
    virtual bool postGenerateFiles(const QWizard *w,
        const Core::GeneratedFiles &l, QString *errorMessage);

    virtual AbstractMobileApp *app() const = 0;
    virtual AbstractMobileAppWizardDialog *wizardDialog() const = 0;
    virtual AbstractMobileAppWizardDialog *createWizardDialogInternal(QWidget *parent,
                                                                      const Core::WizardDialogParameters &wizardDialogParameters) const = 0;
    virtual void projectPathChanged(const QString &path) const = 0;
    virtual void prepareGenerateFiles(const QWizard *wizard,
        QString *errorMessage) const = 0;
};

} // namespace Qt4ProjectManager

#endif // ABSTRACTMOBILEAPPWIZARD_H
