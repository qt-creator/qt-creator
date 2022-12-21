// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId, const QVariant &data) override;

    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

// A wizard page embedding Qt Designer's QDesignerNewFormWidgetInterface
// widget.

// Sets FormContents property.

class FormTemplateWizardPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    explicit FormTemplateWizardPage(QWidget *parent = nullptr);

    bool isComplete () const override;
    bool validatePage() override;

    QString templateContents() const { return  m_templateContents; }

    static QString stripNamespaces(const QString &className);

signals:
    void templateActivated();

private:
    void slotCurrentTemplateChanged(bool);

    QString m_templateContents;
    QDesignerNewFormWidgetInterface *m_newFormWidget = nullptr;
    bool m_templateSelected = false;
};

} // namespace Internal
} // namespace Designer
