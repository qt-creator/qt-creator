/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMMONOPTIONSPAGE_H
#define COMMONOPTIONSPAGE_H

#include "commonvcssettings.h"

#include "vcsbaseoptionspage.h"

#include <QtCore/QPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CommonSettingsPage;
}
QT_END_NAMESPACE

namespace VCSBase {
namespace Internal {

class CommonSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommonSettingsWidget(QWidget *parent = 0);
    virtual ~CommonSettingsWidget();

    CommonVcsSettings settings() const;
    void setSettings(const CommonVcsSettings &s);

    QString searchKeyWordMatchString() const;

private:
    Ui::CommonSettingsPage *m_ui;
};

class CommonOptionsPage : public VCSBaseOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(QObject *parent = 0);
    virtual ~CommonOptionsPage();

    virtual QString id() const;
    virtual QString displayName() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }
    virtual bool matches(const QString &key) const;

    CommonVcsSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VCSBase::Internal::CommonVcsSettings &s);

private:
    void updateNickNames();

    CommonSettingsWidget* m_widget;
    CommonVcsSettings m_settings;
    QString m_searchKeyWords;
};

} // namespace Internal
} // namespace VCSBase

#endif // COMMONOPTIONSPAGE_H
