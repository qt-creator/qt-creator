// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace BareMetal::Internal {

class IDebugServerProvider;

class DebugServerProviderChooser final : public QWidget
{
    Q_OBJECT

public:
    explicit DebugServerProviderChooser(
            bool useManageButton = true, QWidget *parent = nullptr);

    QString currentProviderId() const;
    void setCurrentProviderId(const QString &id);
    void populate();

signals:
    void providerChanged();

private:
    void currentIndexChanged(int index);
    void manageButtonClicked();
    bool providerMatches(const IDebugServerProvider *) const;
    QString providerText(const IDebugServerProvider *) const;

    QComboBox *m_chooser = nullptr;
    QPushButton *m_manageButton = nullptr;
};

} // BareMetal::Internal
