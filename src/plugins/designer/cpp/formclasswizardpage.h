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

#ifndef FORMCLASSWIZARDPAGE_H
#define FORMCLASSWIZARDPAGE_H

#include <QWizardPage>

namespace Designer {

class FormClassWizardParameters;
class FormClassWizardGenerationParameters;

namespace Internal {

namespace Ui {
class FormClassWizardPage;
}


class FormClassWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit FormClassWizardPage(QWidget * parent = 0);
    ~FormClassWizardPage();

    virtual bool isComplete () const;
    virtual bool validatePage();

    QString path() const;

    // Fill out applicable parameters
    void getParameters(FormClassWizardParameters *) const;

    FormClassWizardGenerationParameters generationParameters() const;
    void setGenerationParameters(const FormClassWizardGenerationParameters &gp);

    static bool lowercaseHeaderFiles();

public slots:
    void setClassName(const QString &suggestedClassName);
    void setPath(const QString &);

private slots:
    void slotValidChanged();

private:
    void initFileGenerationSettings();

    Ui::FormClassWizardPage *m_ui;
    bool m_isValid;
};

} // namespace Internal
} // namespace Designer

#endif //FORMCLASSWIZARDPAGE_H
