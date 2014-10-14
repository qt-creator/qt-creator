/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MEMCHECKTOOL_H
#define MEMCHECKTOOL_H

#include "valgrindtool.h"

#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {
class ErrorListModel;
class Error;
}
}

namespace Valgrind {
namespace Internal {

class FrameFinder;
class MemcheckErrorView;
class ValgrindBaseSettings;

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

private slots:
    void settingsDestroyed(QObject *settings);
    void maybeActiveRunConfigurationChanged();

    void engineStarting(const Analyzer::AnalyzerRunControl *engine);
    void engineFinished();
    void loadingExternalXmlLogFileFinished();

    void parserError(const Valgrind::XmlProtocol::Error &error);
    void internalParserError(const QString &errorString);
    void updateErrorFilter();
    void suppressionActionTriggered();

    void loadExternalXmlLogFile();

private:
    QWidget *createWidgets();
    void setBusyCursor(bool busy);

    Analyzer::AnalyzerRunControl *createRunControl(const Analyzer::AnalyzerStartParameters &sp,
                               ProjectExplorer::RunConfiguration *runConfiguration = 0);

    void clearErrorView();
    void updateFromSettings();
    int updateUiAfterFinishedHelper();

private:
    ValgrindBaseSettings *m_settings;
    QMenu *m_filterMenu;

    FrameFinder *m_frameFinder;
    Valgrind::XmlProtocol::ErrorListModel *m_errorModel;
    MemcheckErrorFilterProxyModel *m_errorProxyModel;
    MemcheckErrorView *m_errorView;

    QList<QAction *> m_errorFilterActions;
    QAction *m_filterProjectAction;
    QList<QAction *> m_suppressionActions;
    QAction *m_suppressionSeparator;
    QAction *m_loadExternalLogFile;
    QAction *m_goBack;
    QAction *m_goNext;
};

} // namespace Internal
} // namespace Valgrind

#endif // MEMCHECKTOOL_H
