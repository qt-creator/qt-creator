/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
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

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "settings.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace Todo {
namespace Internal {

class OptionsDialog;

class OptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    OptionsPage(const Settings &settings, QObject *parent = 0);

    void setSettings(const Settings &settings);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &searchKeyWord) const;

signals:
    void settingsChanged(const Settings &settings);

private:
    OptionsDialog *m_dialog;
    Settings m_settings;
};

} // namespace Internal
} // namespace Todo

#endif // SETTINGSPAGE_H
