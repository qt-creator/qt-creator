/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COMMONSETTINGSPAGE_H
#define COMMONSETTINGSPAGE_H

#include "commonvcssettings.h"

#include "vcsbaseoptionspage.h"

#include <QPointer>
#include <QWidget>

namespace VcsBase {
namespace Internal {

namespace Ui { class CommonSettingsPage; }

class CommonSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommonSettingsWidget(QWidget *parent = 0);
    ~CommonSettingsWidget();

    CommonVcsSettings settings() const;
    void setSettings(const CommonVcsSettings &s);

private slots:
    void updatePath();

private:
    Ui::CommonSettingsPage *m_ui;
};

class CommonOptionsPage : public VcsBaseOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(QObject *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

    CommonVcsSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);

private:
    QPointer<CommonSettingsWidget> m_widget;
    CommonVcsSettings m_settings;
};

} // namespace Internal
} // namespace VcsBase

#endif // COMMONSETTINGSPAGE_H
