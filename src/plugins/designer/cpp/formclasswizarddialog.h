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

#ifndef FORMCLASSWIZARDDIALOG_H
#define FORMCLASSWIZARDDIALOG_H

#include <QtGui/QWizard>

namespace Core {
    class ICore;
}

namespace Designer {
namespace Internal {

struct FormClassWizardParameters;
class FormClassWizardPage;
class FormTemplateWizardPage;

class FormClassWizardDialog : public QWizard
{
    Q_DISABLE_COPY(FormClassWizardDialog)
    Q_OBJECT

public:
    typedef QList<QWizardPage *> WizardPageList;

    explicit FormClassWizardDialog(const WizardPageList &extensionPages,
                                   QWidget *parent = 0);

    void setSuffixes(const QString &header, const QString &source,  const QString &form);

    QString path() const;

    FormClassWizardParameters parameters() const;

    bool validateCurrentPage();

public slots:
    void setPath(const QString &);

private slots:
    void slotCurrentIdChanged(int id);

private:
    FormTemplateWizardPage *m_formPage;
    FormClassWizardPage *m_classPage;
    QString m_rawFormTemplate;
};

} // namespace Internal
} // namespace Designer

#endif // FORMCLASSWIZARDDIALOG_H
