/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    DebuggerKitConfigWidget(ProjectExplorer::Kit *p,
                            const DebuggerKitInformation *ki,
                            QWidget *parent = 0);

    QString displayName() const;

    void makeReadOnly();

    void apply();
    void discard();
    bool isDirty() const { return m_dirty; }
    QWidget *buttonWidget() const;

private slots:
    void autoDetectDebugger();
    void showDialog();

private:
    void setItem(const DebuggerKitInformation::DebuggerItem &item);
    void doSetItem(const DebuggerKitInformation::DebuggerItem &item);

    ProjectExplorer::Kit *m_kit;
    const DebuggerKitInformation *m_info;
    DebuggerKitInformation::DebuggerItem m_item;
    bool m_dirty;
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
