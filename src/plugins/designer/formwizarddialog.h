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

#ifndef FORMWIZARDDIALOG_H
#define FORMWIZARDDIALOG_H

#include <utils/wizard.h>

namespace Utils {
    class FileWizardPage;
}

namespace Designer {
namespace Internal {

class FormTemplateWizardPage;

// Single-Page Wizard for new forms offering all types known to Qt Designer.
// To be used for Mode "CreateNewEditor" [not currently used]

class FormWizardDialog : public Utils::Wizard
{
    Q_OBJECT

public:
    typedef QList<QWizardPage *> WizardPageList;
    explicit FormWizardDialog(const WizardPageList &extensionPages,
                              QWidget *parent = 0);

    QString templateContents() const;

private:
    void init(const WizardPageList &extensionPages);

    FormTemplateWizardPage *m_formPage;
    mutable QString m_templateContents;
};

// Two-Page Wizard for new forms for mode "CreateNewFile". Gives
// FormWizardDialog an additional page with file and path fields,
// initially determined from the UI class chosen on page one.

class FormFileWizardDialog : public FormWizardDialog
{
    Q_OBJECT

public:
    explicit FormFileWizardDialog(const WizardPageList &extensionPages,
                                  QWidget *parent = 0);

    QString path() const;
    QString fileName() const;

public slots:
    void setPath(const QString &path);

private slots:
    void slotCurrentIdChanged(int id);

private:
    Utils::FileWizardPage *m_filePage;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWIZARDDIALOG_H
