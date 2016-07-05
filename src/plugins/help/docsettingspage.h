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

#pragma once

#include "ui_docsettingspage.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QList>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Help {
namespace Internal {

class DocModel;

class DocSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DocSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    void addDocumentation();

    bool eventFilter(QObject *object, QEvent *event);
    void removeDocumentation(const QList<QModelIndex> &items);

    QList<QModelIndex> currentSelection() const;

    Ui::DocSettingsPage m_ui;
    QPointer<QWidget> m_widget;

    QString m_recentDialogPath;

    typedef QHash<QString, QString> NameSpaceToPathHash;
    NameSpaceToPathHash m_filesToRegister;
    QHash<QString, bool> m_filesToRegisterUserManaged;
    NameSpaceToPathHash m_filesToUnregister;

    QSortFilterProxyModel *m_proxyModel = nullptr;
    DocModel *m_model = nullptr;
};

} // namespace Help
} // namespace Internal
