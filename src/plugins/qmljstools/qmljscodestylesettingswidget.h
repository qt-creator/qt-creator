// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <QGroupBox>

namespace QmlJSTools {
class QmlJSCodeStyleSettings;

namespace Internal { namespace Ui { class QmlJSCodeStyleSettingsWidget; } }


class QMLJSTOOLS_EXPORT QmlJSCodeStyleSettingsWidget : public QGroupBox
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    explicit QmlJSCodeStyleSettingsWidget(QWidget *parent = nullptr);
    ~QmlJSCodeStyleSettingsWidget() override;

    QmlJSCodeStyleSettings codeStyleSettings() const;

    void setCodingStyleWarningVisible(bool visible);
    void setCodeStyleSettings(const QmlJSCodeStyleSettings& s);

signals:
    void settingsChanged(const QmlJSCodeStyleSettings &);

private:
    void slotSettingsChanged();
    void codingStyleLinkActivated(const QString &linkString);

    Internal::Ui::QmlJSCodeStyleSettingsWidget *ui;
};

} // namespace QmlJSTools
