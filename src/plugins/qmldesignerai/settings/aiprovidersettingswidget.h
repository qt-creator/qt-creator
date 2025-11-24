// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include <utils/uniqueobjectptr.h>

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

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

protected:
    bool event(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setupUi();

    AiProviderConfig m_config;
    Utils::UniqueObjectPtr<QLineEdit> m_urlLineEdit;
    Utils::UniqueObjectPtr<QLineEdit> m_apiKeyLineEdit;
    Utils::UniqueObjectPtr<StringListWidget> m_modelsListWidget;
    bool m_mouseOverCheckBox = false;
};

} // namespace QmlDesigner
