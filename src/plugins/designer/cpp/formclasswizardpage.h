/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QWizardPage>

namespace Designer {

class FormClassWizardParameters;
class FormClassWizardGenerationParameters;

namespace Internal {

namespace Ui { class FormClassWizardPage; }


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
