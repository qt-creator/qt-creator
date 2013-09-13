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

class DebuggerItem;
class DebuggerItemConfigWidget;
class DebuggerKitConfigWidget;

// -----------------------------------------------------------------------
// DebuggerItemManager
// -----------------------------------------------------------------------

class DebuggerItemManager : public QStandardItemModel
{
    Q_OBJECT

public:
    DebuggerItemManager(QObject *parent);
    ~DebuggerItemManager();

    static const DebuggerItem *debuggerFromKit(const ProjectExplorer::Kit *kit);
    void setDebugger(ProjectExplorer::Kit *kit,
        DebuggerEngineType type, const Utils::FileName &command);

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);
    void setCurrentData(const QString &displayName, const Utils::FileName &fileName);

    // Returns id.
    QVariant defaultDebugger(ProjectExplorer::ToolChain *tc);
    static void restoreDebuggers();

public slots:
    void saveDebuggers();
    void autoDetectDebuggers();
    void readLegacyDebuggers();
    void addDebugger();
    void cloneDebugger();
    void removeDebugger();
    void markCurrentDirty();

signals:
    void debuggerAdded(const QVariant &id, const QString &display);
    void debuggerUpdated(const QVariant &id, const QString &display);
    void debuggerRemoved(const QVariant &id);

private:
    friend class Debugger::DebuggerKitInformation;
    friend class DebuggerKitConfigWidget;
    friend class DebuggerItemConfigWidget;
    friend class DebuggerOptionsPage;

    QVariant doAddDebugger(const DebuggerItem &item);
    const DebuggerItem *findByCommand(const Utils::FileName &command);
    const DebuggerItem *findById(const QVariant &id);
    QStandardItem *currentStandardItem() const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QString uniqueDisplayName(const QString &base) const;
    void autoDetectCdbDebugger();

    Utils::PersistentSettingsWriter *m_writer;
    QList<DebuggerItem> m_debuggers;
    QVariant m_currentDebugger;

    QStandardItem *m_autoRoot;
    QStandardItem *m_manualRoot;
    QStringList removed;
};

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
    void onDebuggerAdded(const QVariant &id, const QString &displayName);
    void onDebuggerUpdated(const QVariant &id, const QString &displayName);
    void onDebuggerRemoved(const QVariant &id);

private:
    int indexOf(const QVariant &id);
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
