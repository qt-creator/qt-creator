/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CALLGRINDTOOL_H
#define CALLGRINDTOOL_H

#include <analyzerbase/ianalyzertool.h>

#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class Function;
class ParseData;
}
}

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IEditor;
}

namespace Callgrind {
namespace Internal {

class CallgrindEngine;
class CallgrindWidgetHandler;
class CallgrindOutputPaneAdapter;
class CallgrindTextMark;

class CallgrindTool : public Analyzer::IAnalyzerTool
{
    Q_OBJECT

public:
    explicit CallgrindTool(QObject *parent = 0);
    virtual ~CallgrindTool();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual ToolMode mode() const;

    virtual void initialize();
    virtual void extensionsInitialized();
    virtual void initializeDockWidgets();

    virtual Analyzer::IAnalyzerEngine *createEngine(const Analyzer::AnalyzerStartParameters &sp,
                                          ProjectExplorer::RunConfiguration *runConfiguration = 0);
    virtual QWidget *createControlWidget();

    // For the output pane adapter.
    CallgrindWidgetHandler *callgrindWidgetHandler() const;
    void clearErrorView();

    virtual bool canRunRemotely() const;

signals:
    void dumpRequested();
    void resetRequested();
    void pauseToggled(bool checked);

    void profilingStartRequested(const QString &toggleCollectFunction);

private slots:
    void showParserResults(const Valgrind::Callgrind::ParseData *data);

    void slotRequestDump();
    void slotFunctionSelected(const Valgrind::Callgrind::Function *);

    void editorOpened(Core::IEditor *);
    void requestContextMenu(TextEditor::ITextEditor *editor, int line, QMenu *menu);

    void handleShowCostsAction();
    void handleShowCostsOfFunction();

    void takeParserData(CallgrindEngine *engine);

    void engineStarting(const Analyzer::IAnalyzerEngine *);
    void engineFinished();

private:
    /// This function will add custom text marks to the editor
    /// \note Call this after the data model has been populated
    void createTextMarks();

    /// This function will clear all text marks from the editor
    void clearTextMarks();

    CallgrindWidgetHandler *m_callgrindWidgetHandler;
    QVector<CallgrindTextMark *> m_textMarks;

    QAction *m_dumpAction;
    QAction *m_resetAction;
    QAction *m_pauseAction;

    QAction *m_showCostsOfFunctionAction;
    QWidget *m_toolbarWidget;

    QString m_toggleCollectFunction;
};

} // namespace Internal
} // namespace Callgrind

#endif // CALLGRINDTOOL_H
