/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FORMWIZARDDIALOG_H
#define FORMWIZARDDIALOG_H

#include <QtGui/QWizard>

namespace Core {
namespace Utils {
    class FileWizardPage;
}
}

namespace Designer {
namespace Internal {

class FormTemplateWizardPage;

// Single-Page Wizard for new forms offering all types known to Qt Designer.
// To be used for Mode "CreateNewEditor" [not currently used]

class FormWizardDialog : public QWizard
{
    Q_DISABLE_COPY(FormWizardDialog)
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
    Q_DISABLE_COPY(FormFileWizardDialog)
    Q_OBJECT

public:
    explicit FormFileWizardDialog(const WizardPageList &extensionPages,
                                  QWidget *parent = 0);

    QString path() const;
    QString name() const;

public slots:
    void setPath(const QString &path);

private slots:
    void slotCurrentIdChanged(int id);

private:
    Core::Utils::FileWizardPage *m_filePage;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWIZARDDIALOG_H
