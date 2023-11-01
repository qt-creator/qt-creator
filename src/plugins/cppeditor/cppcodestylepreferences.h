// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppcodestylesettings.h"

#include <texteditor/icodestylepreferences.h>

namespace CppEditor {

class CPPEDITOR_EXPORT CppCodeStylePreferences : public TextEditor::ICodeStylePreferences
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferences(
        QObject *parent = nullptr);

    QVariant value() const override;
    void setValue(const QVariant &) override;

    CppCodeStyleSettings codeStyleSettings() const;

    // tracks parent hierarchy until currentParentSettings is null
    CppCodeStyleSettings currentCodeStyleSettings() const;

    Utils::Store toMap() const override;
    void fromMap(const Utils::Store &map) override;

public slots:
    void setCodeStyleSettings(const CppCodeStyleSettings &data);

signals:
    void codeStyleSettingsChanged(const CppCodeStyleSettings &);
    void currentCodeStyleSettingsChanged(const CppCodeStyleSettings &);

private:
    void slotCurrentValueChanged(const QVariant &);

    CppCodeStyleSettings m_data;
};

} // namespace CppEditor
