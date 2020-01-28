/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"
#include "cppcodestylesettings.h"

#include <texteditor/icodestylepreferences.h>

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStylePreferences : public TextEditor::ICodeStylePreferences
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

    void toMap(const QString &prefix, QVariantMap *map) const override;
    void fromMap(const QString &prefix, const QVariantMap &map) override;

public slots:
    void setCodeStyleSettings(const CppTools::CppCodeStyleSettings &data);

signals:
    void codeStyleSettingsChanged(const CppTools::CppCodeStyleSettings &);
    void currentCodeStyleSettingsChanged(const CppTools::CppCodeStyleSettings &);

private:
    void slotCurrentValueChanged(const QVariant &);

    CppCodeStyleSettings m_data;
};

} // namespace CppTools
