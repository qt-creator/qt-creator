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

#ifndef DEBUGGER_DEBUGGEROPTIONSPAGE_H
#define DEBUGGER_DEBUGGEROPTIONSPAGE_H

#include "debuggeritem.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
} // namespace Utils

namespace Debugger {
namespace Internal {

class DebuggerItemModel;
class DebuggerItemConfigWidget;
class DebuggerKitConfigWidget;

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DebuggerItemConfigWidget(DebuggerItemModel *model);
    void setItem(const DebuggerItem &item);
    void apply();

private slots:
    void binaryPathHasChanged();

private:
    DebuggerItem item() const;
    void store() const;
    void setAbis(const QStringList &abiNames);

    void handleCommandChange();

    QLineEdit *m_displayNameLineEdit;
    QLabel *m_cdbLabel;
    Utils::PathChooser *m_binaryChooser;
    QLineEdit *m_abis;
    DebuggerItemModel *m_model;
    bool m_autodetected;
    DebuggerEngineType m_engineType;
    QVariant m_id;
};

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

class DebuggerOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DebuggerOptionsPage();

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void debuggerSelectionChanged();
    void debuggerModelChanged();
    void updateState();
    void cloneDebugger();
    void addDebugger();
    void removeDebugger();

private:
    QPointer<QWidget> m_configWidget;

    DebuggerItemModel *m_model;
    DebuggerItemConfigWidget *m_itemConfigWidget;
    QTreeView *m_debuggerView;
    Utils::DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGEROPTIONSPAGE_H
