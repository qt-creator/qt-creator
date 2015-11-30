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

#ifndef VCSBASEOPTIONSPAGE_H
#define VCSBASEOPTIONSPAGE_H

#include "vcsbase_global.h"

#include "vcsbaseclientsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QWidget>

#include <functional>

namespace Core { class IVersionControl; }

namespace VcsBase {

class VCSBASE_EXPORT VcsBaseOptionsPage : public Core::IOptionsPage
{
public:
    explicit VcsBaseOptionsPage(QObject *parent = 0);
    ~VcsBaseOptionsPage();
};

class VcsBaseClientImpl;

class VCSBASE_EXPORT VcsClientOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    VcsClientOptionsPageWidget(QWidget *parent = 0);

    virtual void setSettings(const VcsBaseClientSettings &s) = 0;
    virtual VcsBaseClientSettings settings() const = 0;
};

class VCSBASE_EXPORT VcsClientOptionsPage : public VcsBaseOptionsPage
{
    Q_OBJECT

public:
    using WidgetFactory = std::function<VcsClientOptionsPageWidget *()>;

    explicit VcsClientOptionsPage(Core::IVersionControl *control, VcsBaseClientImpl *client, QObject *parent = 0);

    VcsClientOptionsPageWidget *widget();
    virtual void apply();
    virtual void finish();

signals:
    void settingsChanged();

protected:
    void setWidgetFactory(WidgetFactory factory);

private:
    WidgetFactory m_factory;
    VcsClientOptionsPageWidget *m_widget;
    VcsBaseClientImpl *const m_client;
};

} // namespace VcsBase

#endif // VCSBASEOPTIONSPAGE_H
