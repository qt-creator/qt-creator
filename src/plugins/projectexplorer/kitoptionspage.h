/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITOPTIONSPAGE_H
#define KITOPTIONSPAGE_H

#include "projectexplorer_export.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QModelIndex>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QItemSelectionModel;
class QTreeView;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

namespace Internal { class KitModel; }

class Kit;

// --------------------------------------------------------------------------
// KitOptionsPage:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT KitOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    KitOptionsPage();

    QWidget *widget();
    void apply();
    void finish();

    void showKit(Kit *k);

private slots:
    void kitSelectionChanged();
    void addNewKit();
    void cloneKit();
    void removeKit();
    void makeDefaultKit();
    void updateState();

private:
    QModelIndex currentIndex() const;

    QTreeView *m_kitsView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_makeDefaultButton;

    QPointer<QWidget> m_configWidget;

    Internal::KitModel *m_model;
    QItemSelectionModel *m_selectionModel;
    QWidget *m_currentWidget;

    Kit *m_toShow;
};

} // namespace ProjectExplorer

#endif // KITOPTIONSPAGE_H
