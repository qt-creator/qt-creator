// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filenamingparameters.h"

#include <QWizardPage>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

struct PluginOptions;
class CustomWidgetWidgetsWizardPage;

class CustomWidgetPluginWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CustomWidgetPluginWizardPage(QWidget *parent = nullptr);

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

    FileNamingParameters m_fileNamingParameters;
    int m_classCount;
    bool m_complete;

    QLabel *m_collectionClassLabel;
    QLineEdit *m_collectionClassEdit;
    QLabel *m_collectionHeaderLabel;
    QLineEdit *m_collectionHeaderEdit;
    QLabel *m_collectionSourceLabel;
    QLineEdit *m_collectionSourceEdit;
    QLineEdit *m_pluginNameEdit;
    QLineEdit *m_resourceFileEdit;
};

} // namespace Internal
} // namespace QmakeProjectManager
