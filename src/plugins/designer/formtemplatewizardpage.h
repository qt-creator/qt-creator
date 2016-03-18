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

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/wizardpage.h>

QT_BEGIN_NAMESPACE
class QDesignerNewFormWidgetInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class FormPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    FormPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Core::Id typeId, const QVariant &data) override;

    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) override;
};

// A wizard page embedding Qt Designer's QDesignerNewFormWidgetInterface
// widget.

// Sets FormContents property.

class FormTemplateWizardPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit FormTemplateWizardPage(QWidget * parent = 0);

    bool isComplete () const override;
    bool validatePage() override;

    QString templateContents() const { return  m_templateContents; }

    static QString stripNamespaces(const QString &className);

signals:
    void templateActivated();

private slots:
    void slotCurrentTemplateChanged(bool);

private:
    QString m_templateContents;
    QDesignerNewFormWidgetInterface *m_newFormWidget;
    bool m_templateSelected;
};

} // namespace Internal
} // namespace Designer
