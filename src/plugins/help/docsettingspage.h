/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef DOCSETTINGSPAGE_H
#define DOCSETTINGSPAGE_H

#include "ui_docsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Help {
namespace Internal {

class DocSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DocSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void addDocumentation();
    void removeDocumentation();

private:
    bool eventFilter(QObject *object, QEvent *event);
    void removeDocumentation(const QList<QListWidgetItem *> &items);
    void addItem(const QString &nameSpace, const QString &fileName, bool userManaged);

private:
    Ui::DocSettingsPage m_ui;
    QPointer<QWidget> m_widget;

    QString m_recentDialogPath;

    typedef QHash<QString, QString> NameSpaceToPathHash;
    NameSpaceToPathHash m_filesToRegister;
    QHash<QString, bool> m_filesToRegisterUserManaged;
    NameSpaceToPathHash m_filesToUnregister;
};

} // namespace Help
} // namespace Internal

#endif // DOCSETTINGSPAGE_H
