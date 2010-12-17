/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
