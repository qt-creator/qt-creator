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

#ifndef QUICKTOOLBARSETTINGSPAGE_H
#define QUICKTOOLBARSETTINGSPAGE_H

#include "ui_quicktoolbarsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlJSEditor {

    class QuickToolBarSettings {
    public:
        QuickToolBarSettings();

        static QuickToolBarSettings get();
        void set();

        void fromSettings(QSettings *);
        void toSettings(QSettings *) const;

        bool equals(const QuickToolBarSettings &other) const;
        bool enableContextPane;
        bool pinContextPane;
    };

    inline bool operator==(const QuickToolBarSettings &s1, const QuickToolBarSettings &s2)
    { return s1.equals(s2); }
    inline bool operator!=(const QuickToolBarSettings &s1, const QuickToolBarSettings &s2)
    { return !s1.equals(s2); }


class QuickToolBarSettings;

namespace Internal {

class QuickToolBarSettingsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuickToolBarSettingsPageWidget(QWidget *parent = 0);

    QuickToolBarSettings settings() const;
    void setSettings(const QuickToolBarSettings &);

    static QuickToolBarSettings get();

private:
    Ui::QuickToolBarSettingsPage m_ui;
};


class QuickToolBarSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    QuickToolBarSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<QuickToolBarSettingsPageWidget> m_widget;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QUICKTOOLBARSETTINGSPAGE_H
