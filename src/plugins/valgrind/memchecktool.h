/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef MEMCHECKTOOL_H
#define MEMCHECKTOOL_H

#include "valgrindtool.h"

#include <QSortFilterProxyModel>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QTreeView;
class QModelIndex;
class QAction;
class QSpinBox;
class QCheckBox;
class QMenu;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {
class ErrorListModel;
class Error;
}
}

namespace Analyzer {
class AnalyzerSettings;
}

namespace Valgrind {
namespace Internal {

class MemcheckErrorView;
class FrameFinder;

class MemcheckErrorFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MemcheckErrorFilterProxyModel(QObject *parent = 0);

public slots:
    void setAcceptedKinds(const QList<int> &acceptedKinds);
    void setFilterExternalIssues(bool filter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QList<int> m_acceptedKinds;
    bool m_filterExternalIssues;
};

class MemcheckTool : public ValgrindTool
{
    Q_OBJECT

public:
    MemcheckTool(QObject *parent);

    Core::Id id() const;
    ProjectExplorer::RunMode runMode() const;
    QString displayName() const;
    QString description() const;

    // Create the valgrind settings (for all valgrind tools)
    Analyzer::AbstractAnalyzerSubConfig *createGlobalSettings();
    Analyzer::AbstractAnalyzerSubConfig *createProjectSettings();

private slots:
    void settingsDestroyed(QObject *settings);
    void maybeActiveRunConfigurationChanged();

    void engineStarting(const Analyzer::IAnalyzerEngine *engine);
    void finished();

    void parserError(const Valgrind::XmlProtocol::Error &error);
    void internalParserError(const QString &errorString);
    void updateErrorFilter();
    void suppressionActionTriggered();

private:
    ToolMode toolMode() const;
    void extensionsInitialized() {}
    QWidget *createWidgets();
    void setBusyCursor(bool busy);

    Analyzer::IAnalyzerEngine *createEngine(const Analyzer::AnalyzerStartParameters &sp,
                               ProjectExplorer::RunConfiguration *runConfiguration = 0);
    void startTool(Analyzer::StartMode mode);

    void clearErrorView();

private:
    Analyzer::AnalyzerSettings *m_settings;
    QMenu *m_filterMenu;

    FrameFinder *m_frameFinder;
    Valgrind::XmlProtocol::ErrorListModel *m_errorModel;
    MemcheckErrorFilterProxyModel *m_errorProxyModel;
    MemcheckErrorView *m_errorView;

    QList<QAction *> m_errorFilterActions;
    QAction *m_filterProjectAction;
    QList<QAction *> m_suppressionActions;
    QAction *m_suppressionSeparator;
    QAction *m_goBack;
    QAction *m_goNext;
};

} // namespace Internal
} // namespace Valgrind

#endif // MEMCHECKTOOL_H
