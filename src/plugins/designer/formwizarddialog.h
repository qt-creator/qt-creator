/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef FORMWIZARDDIALOG_H
#define FORMWIZARDDIALOG_H

#include <QtGui/QWizard>

namespace Core {
    class ICore;
    namespace Utils {
        class FileWizardPage;
    }
}

namespace Designer {
namespace Internal {

class FormTemplateWizardPagePage;

// Single-Page Wizard for new forms offering all types known to Qt Designer.
// To be used for Mode "CreateNewEditor" [not currently used]

class FormWizardDialog : public QWizard
{
    Q_DISABLE_COPY(FormWizardDialog)
    Q_OBJECT

public:
    typedef QList<QWizardPage *> WizardPageList;
    explicit FormWizardDialog(Core::ICore *core,
                              const WizardPageList &extensionPages,
                              QWidget *parent = 0);

    QString templateContents() const;

private:
    void init(const WizardPageList &extensionPages);

    FormTemplateWizardPagePage *m_formPage;
    Core::ICore *m_core;
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
    explicit FormFileWizardDialog(Core::ICore *core,
                                  const WizardPageList &extensionPages,
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

#endif //FORMWIZARDDIALOG_H
