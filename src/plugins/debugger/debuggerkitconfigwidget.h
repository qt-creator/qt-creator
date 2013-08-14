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

#include "debuggerkitinformation.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/abi.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>

#include <QDialog>
#include <QStandardItemModel>
#include <QTreeView>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerItemConfigWidget;

// -----------------------------------------------------------------------
// DebuggerItemManager
// -----------------------------------------------------------------------

class DebuggerItemManager : public QStandardItemModel
{
    Q_OBJECT

public:
    DebuggerItemManager(QObject *parent);
    ~DebuggerItemManager();

    QList<DebuggerItem *> debuggers() const { return m_debuggers; }
    QList<DebuggerItem *> findDebuggers(const ProjectExplorer::Abi &abi) const;
    DebuggerItem *currentDebugger() const { return m_currentDebugger; }
    static DebuggerItem *debuggerFromId(const QVariant &id);
    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    bool isLoaded() const;
    void updateCurrentItem();
    // Returns id.
    QVariant defaultDebugger(ProjectExplorer::ToolChain *tc);
    // Adds item unless present. Return id of a matching item.
    QVariant maybeAddDebugger(const DebuggerItem &item, bool makeCurrent = true);
    static void restoreDebuggers();

public slots:
    void saveDebuggers();
    void autoDetectDebuggers();
    void addDebugger();
    void cloneDebugger();
    void removeDebugger();

signals:
    void debuggerAdded(DebuggerItem *item);
    void debuggerRemoved(DebuggerItem *item);
    void debuggerUpdated(DebuggerItem *item);

private:
    friend class DebuggerOptionsPage;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void autoDetectCdbDebugger();

    QMap<QString, Utils::FileName> m_abiToDebugger;
    Utils::PersistentSettingsWriter *m_writer;
    QList<DebuggerItem *> m_debuggers;
    DebuggerItem *m_currentDebugger;

    QStandardItem *m_autoRoot;
    QStandardItem *m_manualRoot;
    QStringList added;
    QStringList removed;
    QHash<DebuggerItem *, QStandardItem *> m_itemFromDebugger;
    QHash<QStandardItem *, DebuggerItem *> m_debuggerFromItem;
};

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget
// -----------------------------------------------------------------------

class DebuggerKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    DebuggerKitConfigWidget(ProjectExplorer::Kit *workingCopy, bool sticky);

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    QWidget *buttonWidget() const;
    QWidget *mainWidget() const;

private slots:
    void manageDebuggers();
    void currentDebuggerChanged(int idx);
    void onDebuggerAdded(DebuggerItem *);
    void onDebuggerUpdated(DebuggerItem *);
    void onDebuggerRemoved(DebuggerItem *);

private:
    int indexOf(const DebuggerItem *debugger);
    QVariant currentId() const;
    void updateComboBox(const QVariant &id);

    bool m_isReadOnly;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

class DebuggerOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DebuggerOptionsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &) const;

private slots:
    void debuggerSelectionChanged();
    void debuggerModelChanged();
    void updateState();
    void cloneDebugger();
    void addDebugger();
    void removeDebugger();

private:
    QWidget *m_configWidget;
    QString m_searchKeywords;

    DebuggerItemManager *m_manager;
    DebuggerItemConfigWidget *m_itemConfigWidget;
    QTreeView *m_debuggerView;
    Utils::DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERKITINFORMATION_H
