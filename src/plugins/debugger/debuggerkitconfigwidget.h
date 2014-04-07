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

#ifndef DEBUGGER_DEBUGGERKITCONFIGWIDGET_H
#define DEBUGGER_DEBUGGERKITCONFIGWIDGET_H

#include "debuggeritemmodel.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/abi.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget
// -----------------------------------------------------------------------

class DebuggerKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    DebuggerKitConfigWidget(ProjectExplorer::Kit *workingCopy,
                            const ProjectExplorer::KitInformation *ki);
    ~DebuggerKitConfigWidget();

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    QWidget *buttonWidget() const;
    QWidget *mainWidget() const;

private slots:
    void manageDebuggers();
    void currentDebuggerChanged(int idx);
    void onDebuggerAdded(const QVariant &id);
    void onDebuggerUpdated(const QVariant &id);
    void onDebuggerRemoved(const QVariant &id);

private:
    int indexOf(const QVariant &id);
    QVariant currentId() const;
    void updateComboBox(const QVariant &id);

    bool m_isReadOnly;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERKITCONFIGWIDGET_H
