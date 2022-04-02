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

    QVariantMap toMap() const override;
    void fromMap(const QVariantMap &map) override;

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
