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

#include "ui_qmljseditingsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlJSEditor {

    class QmlJsEditingSettings {
    public:
        QmlJsEditingSettings();

        static QmlJsEditingSettings get();
        void set();

        void fromSettings(QSettings *);
        void toSettings(QSettings *) const;

        bool equals(const QmlJsEditingSettings &other) const;

        bool enableContextPane() const;
        void setEnableContextPane(const bool enableContextPane);

        bool pinContextPane() const;
        void setPinContextPane(const bool pinContextPane);

        bool autoFormatOnSave() const;
        void setAutoFormatOnSave(const bool autoFormatOnSave);

        bool autoFormatOnlyCurrentProject() const;
        void setAutoFormatOnlyCurrentProject(const bool autoFormatOnlyCurrentProject);

        bool foldAuxData() const;
        void setFoldAuxData(const bool foldAuxData);

    private:
        bool m_enableContextPane;
        bool m_pinContextPane;
        bool m_autoFormatOnSave;
        bool m_autoFormatOnlyCurrentProject;
        bool m_foldAuxData;
    };

    inline bool operator==(const QmlJsEditingSettings &s1, const QmlJsEditingSettings &s2)
    { return s1.equals(s2); }
    inline bool operator!=(const QmlJsEditingSettings &s1, const QmlJsEditingSettings &s2)
    { return !s1.equals(s2); }


namespace Internal {

class QmlJsEditingSettingsPage : public Core::IOptionsPage
{
public:
    QmlJsEditingSettingsPage();
};

} // namespace Internal
} // namespace QmlDesigner
