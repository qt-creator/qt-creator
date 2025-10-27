// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include <utils/uniqueobjectptr.h>

#include <QGroupBox>

Q_FORWARD_DECLARE_OBJC_CLASS(QLineEdit);
Q_FORWARD_DECLARE_OBJC_CLASS(QPushButton);

namespace QmlDesigner {

class StringListWidget;

// Settings group box for a single AI provider
class AiProviderSettingsWidget : public QGroupBox
{
public:
    explicit AiProviderSettingsWidget(const QString &providerName, QWidget *parent);

    void load();
    bool save();

    const AiProviderConfig config() const;

private:
    void setupUi();

    AiProviderConfig m_config;
    Utils::UniqueObjectPtr<QLineEdit> m_url;
    Utils::UniqueObjectPtr<QLineEdit> m_apiKey;
    Utils::UniqueObjectPtr<StringListWidget> m_models;
};

} // namespace QmlDesigner
