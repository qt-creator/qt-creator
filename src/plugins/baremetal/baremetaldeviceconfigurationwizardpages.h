// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class DebugServerProviderChooser;

// BareMetalDeviceConfigurationWizardSetupPage

class BareMetalDeviceConfigurationWizardSetupPage final : public QWizardPage
{
    Q_OBJECT

public:
    explicit BareMetalDeviceConfigurationWizardSetupPage(QWidget *parent = nullptr);

    void initializePage() final;
    bool isComplete() const final;
    QString configurationName() const;
    QString debugServerProviderId() const;

private:
    QLineEdit *m_nameLineEdit = nullptr;
    DebugServerProviderChooser *m_debugServerProviderChooser = nullptr;
};

} // namespace Internal
} // namespace BareMetal
