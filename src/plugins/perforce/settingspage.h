/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QtCore/QPointer>
#include <QtGui/QWidget>

#include <coreplugin/dialogs/ioptionspage.h>

#include "ui_settingspage.h"

namespace Perforce {
namespace Internal {

struct PerforceSettings;

class SettingsPageWidget : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPageWidget(QWidget *parent);

    QString p4Command() const;
    bool defaultEnv() const;
    QString p4Port() const;
    QString p4User() const;
    QString p4Client() const;

    void setSettings(const PerforceSettings &);

private:
    Ui::SettingsPage m_ui;
};

class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    SettingsPage();

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

private:
    QPointer<SettingsPageWidget> m_widget;
};

} // namespace Internal
} // namespace Perforce

#endif // SETTINGSPAGE_H
