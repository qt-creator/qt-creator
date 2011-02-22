/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEGUBBER_DEBUGGERCHOOSERWIDGET_H
#define DEGUBBER_DEBUGGERCHOOSERWIDGET_H

#include <QtCore/QMultiMap>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QToolButton;
class QModelIndex;
class QStandardItem;
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QCheckBox;
QT_END_NAMESPACE

namespace Utils {
    class PathChooser;
}

namespace Debugger {
namespace Internal {

class DebuggerBinaryModel;

/* DebuggerChooserWidget: Shows a list of toolchains and associated debugger
 * binaries together with 'add' and 'remove' buttons.
 * Provides delayed tooltip showing version information. */

class DebuggerChooserWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DebuggerChooserWidget(QWidget *parent = 0);

    QMap<QString, QString> debuggerMapping() const;
    void setDebuggerMapping(const QMap<QString, QString> &m);

    bool isDirty() const;

private:
    void removeItem(QStandardItem *item);
    QToolButton *createAddToolMenuButton();

    QTreeView *m_treeView;
    DebuggerBinaryModel *m_model;
};

} // namespace Internal
} // namespace Debugger

#endif // DEGUBBER_DEBUGGERCHOOSERWIDGET_H
