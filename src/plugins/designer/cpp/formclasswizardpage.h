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

#ifndef FORMCLASSWIZARDPAGE_H
#define FORMCLASSWIZARDPAGE_H

#include <QtGui/QWizardPage>

namespace Designer {

class FormClassWizardParameters;
class FormClassWizardGenerationParameters;

namespace Internal {

namespace Ui {
    class FormClassWizardPage;
}


class FormClassWizardPage : public QWizardPage
{
    Q_DISABLE_COPY(FormClassWizardPage)
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

public slots:
    void setClassName(const QString &suggestedClassName);
    void setPath(const QString &);
    void slotSettings();

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
