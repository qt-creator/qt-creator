// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../wizards/qtwizard.h"

#include <QSharedPointer>

namespace QmakeProjectManager {
namespace Internal {

class CustomWidgetWidgetsWizardPage;
class CustomWidgetPluginWizardPage;
struct PluginOptions;
struct FileNamingParameters;

class CustomWidgetWizardDialog : public BaseQmakeProjectWizardDialog
{
    Q_OBJECT
public:
    explicit CustomWidgetWizardDialog(const Core::BaseFileWizardFactory *factory,
                                      const QString &templateName, const QIcon &icon,
                                      QWidget *parent,
                                      const Core::WizardDialogParameters &parameters);

    QSharedPointer<PluginOptions> pluginOptions() const;


    FileNamingParameters fileNamingParameters() const;
    void setFileNamingParameters(const FileNamingParameters &fnp);

private:
    void slotCurrentIdChanged(int id);

    CustomWidgetWidgetsWizardPage *m_widgetsPage;
    CustomWidgetPluginWizardPage *m_pluginPage;
    int m_pluginPageId;
};

} // namespace Internal
} // namespace QmakeProjectManager
