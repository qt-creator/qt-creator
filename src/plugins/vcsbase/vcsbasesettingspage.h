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

#ifndef VCSBASESETTINGSPAGE_H
#define VCSBASESETTINGSPAGE_H

#include "vcsbasesettings.h"
#include <coreplugin/dialogs/ioptionspage.h>
#include <QtCore/QPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class VCSBaseSettingsPage;
}
QT_END_NAMESPACE

namespace VCSBase {
namespace Internal {

class VCSBaseSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit VCSBaseSettingsWidget(QWidget *parent = 0);
    virtual ~VCSBaseSettingsWidget();

    VCSBaseSettings settings() const;
    void setSettings(const VCSBaseSettings &s);

    QString searchKeyWordMatchString() const;

private:
    Ui::VCSBaseSettingsPage *m_ui;
};

class VCSBaseSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit VCSBaseSettingsPage(QObject *parent = 0);
    virtual ~VCSBaseSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }
    virtual bool matches(const QString &key) const;

    VCSBaseSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VCSBase::Internal::VCSBaseSettings &s);

private:
    void updateNickNames();

    VCSBaseSettingsWidget* m_widget;
    VCSBaseSettings m_settings;
    QString m_searchKeyWords;
};

} // namespace Internal
} // namespace VCSBase

#endif // VCSBASESETTINGSPAGE_H
