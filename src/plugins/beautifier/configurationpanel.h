// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Beautifier::Internal {

class AbstractSettings;

class ConfigurationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationPanel(QWidget *parent = nullptr);
    ~ConfigurationPanel() override;

    void setSettings(AbstractSettings *settings);
    void setCurrentConfiguration(const QString &text);
    QString currentConfiguration() const;

private:
    void remove();
    void add();
    void edit();
    void updateButtons();
    void populateConfigurations(const QString &key = QString());

    AbstractSettings *m_settings = nullptr;

    QComboBox *m_configurations;
    QPushButton *m_edit;
    QPushButton *m_remove;
};

} // Beautifier::Internal
