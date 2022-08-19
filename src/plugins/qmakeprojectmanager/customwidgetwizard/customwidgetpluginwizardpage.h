// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "filenamingparameters.h"

#include <QWizardPage>
#include <QSharedPointer>

namespace QmakeProjectManager {
namespace Internal {

struct PluginOptions;
class CustomWidgetWidgetsWizardPage;

namespace Ui { class CustomWidgetPluginWizardPage; }

class CustomWidgetPluginWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CustomWidgetPluginWizardPage(QWidget *parent = nullptr);
    ~CustomWidgetPluginWizardPage() override;

    void init(const CustomWidgetWidgetsWizardPage *widgetsPage);

    bool isComplete() const override;

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) {m_fileNamingParameters = fnp; }

    // Fills the plugin fields, excluding widget list.
    QSharedPointer<PluginOptions> basicPluginOptions() const;

private:
    void slotCheckCompleteness();
    inline QString collectionClassName() const;
    inline QString pluginName() const;
    void setCollectionEnabled(bool enColl);

    Ui::CustomWidgetPluginWizardPage *m_ui;
    FileNamingParameters m_fileNamingParameters;
    int m_classCount;
    bool m_complete;
};

} // namespace Internal
} // namespace QmakeProjectManager
