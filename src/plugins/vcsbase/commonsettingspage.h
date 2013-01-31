/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COMMONOPTIONSPAGE_H
#define COMMONOPTIONSPAGE_H

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

    QString searchKeyWordMatchString() const;

private:
    Ui::CommonSettingsPage *m_ui;
};

class CommonOptionsPage : public VcsBaseOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(QObject *parent = 0);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &key) const;

    CommonVcsSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);

private:
    CommonSettingsWidget *m_widget;
    CommonVcsSettings m_settings;
    QString m_searchKeyWords;
};

} // namespace Internal
} // namespace VcsBase

#endif // COMMONOPTIONSPAGE_H
