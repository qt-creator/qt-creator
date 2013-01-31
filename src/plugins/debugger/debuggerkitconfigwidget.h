/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <projectexplorer/kitconfigwidget.h>

#include "debuggerkitinformation.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer { class Kit; }
namespace Utils {
class PathChooser;
class FileName;
}

namespace Debugger {
class DebuggerKitInformation;

namespace Internal {
// -----------------------------------------------------------------------
// DebuggerKitConfigWidget:
// -----------------------------------------------------------------------

class DebuggerKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    DebuggerKitConfigWidget(ProjectExplorer::Kit *workingCopy);

    QString displayName() const;
    QString toolTip() const;

    void makeReadOnly();

    void refresh();

    QWidget *buttonWidget() const;
    QWidget *mainWidget() const;

private slots:
    void autoDetectDebugger();
    void showDialog();

private:
    QLabel *m_label;
    QPushButton *m_button;
};

class DebuggerKitConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DebuggerKitConfigDialog(QWidget *parent =  0);

    void setDebuggerItem(const DebuggerKitInformation::DebuggerItem &item);
    DebuggerKitInformation::DebuggerItem item() const
        { return DebuggerKitInformation::DebuggerItem(engineType(), fileName()); }

private slots:
    void refreshLabel();

private:
    DebuggerEngineType engineType() const;
    void setEngineType(DebuggerEngineType et);

    Utils::FileName fileName() const;
    void setFileName(const Utils::FileName &fn);

    QComboBox *m_comboBox;
    QLabel *m_label;
    Utils::PathChooser *m_chooser;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERKITINFORMATION_H
