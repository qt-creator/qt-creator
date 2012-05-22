/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/


#ifndef QUICKTOOLBARSETTINGSPAGE_H
#define QUICKTOOLBARSETTINGSPAGE_H

#include "ui_quicktoolbarsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>
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

    QString searchKeywords() const;
    static QuickToolBarSettings get();

private:
    Ui::QuickToolBarSettingsPage m_ui;
};


class QuickToolBarSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    QuickToolBarSettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &) const;

private:
    QString m_searchKeywords;
    QuickToolBarSettingsPageWidget* m_widget;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QUICKTOOLBARSETTINGSPAGE_H
