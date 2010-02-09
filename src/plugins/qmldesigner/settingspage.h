/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/


#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "ui_settingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignerSettings;

namespace Internal {

class SettingsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPageWidget(QWidget *parent = 0);

    DesignerSettings settings() const;
    void setSettings(const DesignerSettings &);

    QString searchKeywords() const;

private:
    Ui::SettingsPage m_ui;
};


class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    SettingsPage();

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    virtual bool matches(const QString &) const;

private:
    QString m_searchKeywords;
    SettingsPageWidget* m_widget;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // SETTINGSPAGE_H
