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

#include "ui_settingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QUrl>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CodePaster {

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

    QString username() const;
    QUrl serverUrl() const;

    inline bool copyToClipBoard() const { return m_copy; }
    inline bool displayOutput() const { return m_output; }

private:
    Ui_SettingsPage m_ui;
    QSettings *m_settings;

    QString m_username;
    QUrl m_server;
    bool m_copy;
    bool m_output;
};

} // namespace CodePaster

#endif // SETTINGSPAGE_H
