/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "ui_optionspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>
#include <QtCore/QPointer>

namespace Mercurial {
namespace Internal {

class MercurialSettings;

class OptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsPageWidget(QWidget *parent = 0);

    MercurialSettings settings() const;
    void setSettings(const MercurialSettings &s);
    QString searchKeywords() const;

private:
    Ui::OptionsPage m_ui;
};


class OptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    OptionsPage();
    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &s) const;

signals:
    void settingsChanged();

private:
    QString m_searchKeywords;
    QPointer<OptionsPageWidget> optionsPageWidget;
};

} // namespace Internal
} // namespace Mercurial

#endif // OPTIONSPAGE_H
