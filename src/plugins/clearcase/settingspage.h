/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "ui_settingspage.h"

#include <vcsbase/vcsbaseoptionspage.h>

#include <QPointer>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ClearCase {
namespace Internal {

class ClearCaseSettings;

class SettingsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget *parent = 0);

    ClearCaseSettings settings() const;
    void setSettings(const ClearCaseSettings &);

    QString searchKeywords() const;

private:
    Ui::SettingsPage m_ui;
};


class SettingsPage : public VcsBase::VcsBaseOptionsPage
{
    Q_OBJECT

public:
    SettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &) const;

private:
    QString m_searchKeywords;
    SettingsPageWidget* m_widget;
};

} // namespace ClearCase
} // namespace Internal

#endif  // SETTINGSPAGE_H
