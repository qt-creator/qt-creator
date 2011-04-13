/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MEMCHECKTOOL_H
#define MEMCHECKTOOL_H

#include <analyzerbase/ianalyzertool.h>

#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QSharedPointer>

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
namespace Internal {
class MemCheckOutputPaneAdapter;
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

class MemcheckTool : public IAnalyzerTool
{
    Q_OBJECT
public:
    explicit MemcheckTool(QObject *parent = 0);

    QString id() const;
    QString displayName() const;
    ToolMode mode() const;

    void initialize(ExtensionSystem::IPlugin *plugin);

    virtual IAnalyzerOutputPaneAdapter *outputPaneAdapter();
    IAnalyzerEngine *createEngine(ProjectExplorer::RunConfiguration *runConfiguration);

    // For the output pane adapter.
    MemcheckErrorView *ensurePaneErrorView();
    QWidget *createPaneToolBarWidget();
    void clearErrorView();

private slots:
    void settingsDestroyed(QObject *settings);
    void maybeActiveRunConfigurationChanged();

    void engineStarting(const IAnalyzerEngine *engine);
    void parserError(const Valgrind::XmlProtocol::Error &error);
    void internalParserError(const QString &errorString);
    void updateErrorFilter();
    void suppressionActionTriggered();
    void finished();
    QMenu *filterMenu() const;

private:
    AnalyzerSettings *m_settings;

    FrameFinder *m_frameFinder;
    Valgrind::XmlProtocol::ErrorListModel *m_errorModel;
    MemcheckErrorFilterProxyModel *m_errorProxyModel;
    MemcheckErrorView *m_errorView;

    QList<QAction *> m_errorFilterActions;
    QAction *m_filterProjectAction;
    QList<QAction *> m_suppressionActions;
    QAction *m_suppressionSeparator;
    MemCheckOutputPaneAdapter *m_outputPaneAdapter;
};

} // namespace Internal
} // namespace Analyzer

#endif // MEMCHECKTOOL_H
