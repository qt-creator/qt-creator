// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include "qmljscodestylesettings.h"

#include <texteditor/icodestylepreferences.h>

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QmlJSCodeStylePreferences : public TextEditor::ICodeStylePreferences
{
    Q_OBJECT
public:
    explicit QmlJSCodeStylePreferences(QObject *parent = nullptr);

    QVariant value() const override;
    void setValue(const QVariant &) override;

    QmlJSCodeStyleSettings codeStyleSettings() const;

    // tracks parent hierarchy until currentParentSettings is null
    QmlJSCodeStyleSettings currentCodeStyleSettings() const;

    Utils::Store toMap() const override;
    void fromMap(const Utils::Store &map) override;

public slots:
    void setCodeStyleSettings(const QmlJSCodeStyleSettings &data);

signals:
    void codeStyleSettingsChanged(const QmlJSCodeStyleSettings &);
    void currentCodeStyleSettingsChanged(const QmlJSCodeStyleSettings &);

private:
    void slotCurrentValueChanged(const QVariant &);

    QmlJSCodeStyleSettings m_data;
};

} // namespace QmlJSTools
