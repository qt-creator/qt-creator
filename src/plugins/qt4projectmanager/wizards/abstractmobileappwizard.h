/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ABSTRACTMOBILEAPPWIZARD_H
#define ABSTRACTMOBILEAPPWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>

namespace Qt4ProjectManager {
namespace Internal {

class AbstractMobileApp;
class MobileAppWizardGenericOptionsPage;
class MobileAppWizardSymbianOptionsPage;
class MobileAppWizardMaemoOptionsPage;

class AbstractMobileAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
protected:
    explicit AbstractMobileAppWizardDialog(QWidget *parent = 0);

public:
    MobileAppWizardGenericOptionsPage *m_genericOptionsPage;
    MobileAppWizardSymbianOptionsPage *m_symbianOptionsPage;
    MobileAppWizardMaemoOptionsPage *m_maemoOptionsPage;
    class TargetSetupPage *m_targetsPage;

protected:
    int addPageWithTitle(QWizardPage *page, const QString &title);
    virtual void initializePage(int id);
    virtual void cleanupPage(int id);

private:
    virtual int nextId() const;

    int idOfNextGenericPage() const;
    Utils::WizardProgressItem *itemOfNextGenericPage() const;

    int m_genericOptionsPageId;
    int m_symbianOptionsPageId;
    int m_maemoOptionsPageId;
    int m_targetsPageId;
    Utils::WizardProgressItem *m_targetItem;
    Utils::WizardProgressItem *m_genericItem;
    Utils::WizardProgressItem *m_symbianItem;
    Utils::WizardProgressItem *m_maemoItem;
};

class AbstractMobileAppWizard : public Core::BaseFileWizard
{
    Q_OBJECT
protected:
    explicit AbstractMobileAppWizard(const Core::BaseFileWizardParameters &params,
        QObject *parent = 0);

private slots:
    void useProjectPath(const QString &projectName, const QString &projectPath);

private:
    virtual QWizard *createWizardDialog(QWidget *parent,
        const QString &defaultPath, const WizardPageList &extensionPages) const;
    virtual Core::GeneratedFiles generateFiles(const QWizard *wizard,
        QString *errorMessage) const;
    virtual bool postGenerateFiles(const QWizard *w,
        const Core::GeneratedFiles &l, QString *errorMessage);

    virtual AbstractMobileApp *app() const=0;
    virtual AbstractMobileAppWizardDialog *wizardDialog() const=0;
    virtual AbstractMobileAppWizardDialog *createWizardDialogInternal(QWidget *parent) const=0;
    virtual void prepareGenerateFiles(const QWizard *wizard,
        QString *errorMessage) const=0;
    virtual bool postGenerateFilesInternal(const Core::GeneratedFiles &l,
        QString *errorMessage)=0;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // ABSTRACTMOBILEAPPWIZARD_H
