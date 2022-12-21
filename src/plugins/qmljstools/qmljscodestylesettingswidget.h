// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE

namespace QmlJSTools {
class QmlJSCodeStyleSettings;

class QMLJSTOOLS_EXPORT QmlJSCodeStyleSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    explicit QmlJSCodeStyleSettingsWidget(QWidget *parent = nullptr);

    QmlJSCodeStyleSettings codeStyleSettings() const;

    void setCodingStyleWarningVisible(bool visible);
    void setCodeStyleSettings(const QmlJSCodeStyleSettings& s);

signals:
    void settingsChanged(const QmlJSCodeStyleSettings &);

private:
    void slotSettingsChanged();
    void codingStyleLinkActivated(const QString &linkString);

    QSpinBox *m_lineLengthSpinBox;
};

} // namespace QmlJSTools
