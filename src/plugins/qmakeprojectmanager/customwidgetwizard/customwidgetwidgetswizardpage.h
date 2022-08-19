// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <QList>
#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QStackedLayout;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

class ClassDefinition;
struct PluginOptions;

namespace Ui { class CustomWidgetWidgetsWizardPage; }

class CustomWidgetWidgetsWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CustomWidgetWidgetsWizardPage(QWidget *parent = nullptr);
    ~CustomWidgetWidgetsWizardPage() override;

    QList<PluginOptions::WidgetOptions> widgetOptions() const;

    bool isComplete() const override;

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) {m_fileNamingParameters = fnp; }

    int classCount() const { return m_uiClassDefs.size(); }
    QString classNameAt(int i) const;

    void initializePage() override;

private Q_SLOTS:
    void slotClassAdded(const QString &name);
    void slotClassDeleted(int index);
    void slotClassRenamed(int index, const QString &newName);
    void slotCheckCompleteness();
    void slotCurrentRowChanged(int);

private:
    void updatePluginTab();

    Ui::CustomWidgetWidgetsWizardPage *m_ui;
    QList<ClassDefinition *> m_uiClassDefs;
    QStackedLayout *m_tabStackLayout;
    FileNamingParameters m_fileNamingParameters;
    bool m_complete;
};

}
}
