/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GDBCHOOSERWIDGET_H
#define GDBCHOOSERWIDGET_H

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

class GdbBinaryModel;

/* GdbChooserWidget: Shows a list of gdb binary and associated toolchains with
 * 'add' and 'remove' buttons. Provides delayed tooltip showing version information.
 * Based on a multimap of binaries to toolchain. */

class GdbChooserWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GdbChooserWidget(QWidget *parent = 0);

    typedef QMultiMap<QString, int> BinaryToolChainMap;

    BinaryToolChainMap gdbBinaries() const;
    void setGdbBinaries(const BinaryToolChainMap &m);

private slots:
    void slotAdd();
    void slotRemove();
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex & previous);
    void slotDoubleClicked(const QModelIndex &current);

private:
    void removeItem(QStandardItem *item);
    QToolButton *createAddToolMenuButton();
    QStandardItem *currentItem() const;

    QTreeView *m_treeView;
    GdbBinaryModel *m_model;
    QToolButton *m_addButton;
    QToolButton *m_deleteButton;
};

// Present toolchains with checkboxes grouped in QGroupBox panes
// and provide valid-handling. Unavailabe toolchains can be grayed
// out using setEnabledToolChains().
class ToolChainSelectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit ToolChainSelectorWidget(QWidget *parent = 0);

    void setEnabledToolChains(const QList<int> &);

    void setCheckedToolChains(const QList<int> &);
    QList<int> checkedToolChains() const;

    bool isValid() const;

signals:
    void validChanged(bool);

private slots:
    void slotCheckStateChanged(int);

private:
    bool hasCheckedToolChain() const;
    QCheckBox *createToolChainCheckBox(int tc);

    QList<QCheckBox*> m_checkBoxes;
    bool m_valid;
};

// Internal helper dialog for selecting a binary and its
// associated toolchains.
class BinaryToolChainDialog : public QDialog {
    Q_OBJECT
public:
    explicit BinaryToolChainDialog(QWidget *parent);

    void setToolChainChoices(const QList<int> &);

    void setToolChains(const QList<int> &);
    QList<int> toolChains() const;

    void setPath(const QString &);
    QString path() const;

private slots:
    void slotValidChanged();

private:
    void setOkButtonEnabled(bool e);

    ToolChainSelectorWidget *m_toolChainSelector;
    QFormLayout *m_mainLayout;
    QDialogButtonBox *m_buttonBox;
    Utils::PathChooser *m_pathChooser;
};

} // namespace Internal
} // namespace Debugger

#endif // GDBCHOOSERWIDGET_H
