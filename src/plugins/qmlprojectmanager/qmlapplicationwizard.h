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

#ifndef QMLAPPLICATIONWIZARD_H
#define QMLAPPLICATIONWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>

namespace ExtensionSystem {
class IPlugin;
} // namespace ExtensionSystem

namespace QmlProjectManager {
namespace Internal {

class QmlApp;
class TemplateInfo;

class QmlApplicationWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
public:
    QmlApplicationWizardDialog(QmlApp *qmlApp, QWidget *parent,
                               const Core::WizardDialogParameters &parameters);

    QmlApp *qmlApp() const;

private:
    QmlApp *m_qmlApp;
};


class QmlApplicationWizard : public Core::BaseFileWizard
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlApplicationWizard)
public:
    QmlApplicationWizard(const Core::BaseFileWizardParameters &parameters,
                             const TemplateInfo &templateInfo, QObject *parent = 0);

    static void createInstances(ExtensionSystem::IPlugin *plugin);


private: // functions
    QWizard *createWizardDialog(QWidget *parent,
                                const Core::WizardDialogParameters &wizardDialogParameters) const;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    void writeUserFile(const QString &fileName) const;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

    static Core::BaseFileWizardParameters parameters();

private: //variables
    mutable QString m_id;
    QmlApp *m_qmlApp;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLAPPLICATIONWIZARD_H
