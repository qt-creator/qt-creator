/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/imode.h>

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QAction;
class QDockWidget;
QT_END_NAMESPACE

namespace Core {
class IMode;
}

namespace Utils {
class FancyMainWindow;
}

namespace Analyzer {

class IAnalyzerTool;

namespace Internal {

class AnalyzerRunControl;

class DockWidgetEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit DockWidgetEventFilter(QObject *parent = 0) : QObject(parent) {}

signals:
    void widgetResized();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
};

class AnalyzerMode : public Core::IMode
{
    Q_OBJECT

public:
    AnalyzerMode(QObject *parent = 0)
        : Core::IMode(parent)
        , m_widget(0)
    {}

    ~AnalyzerMode()
    {
        // Make sure the editor manager does not get deleted.
        if (m_widget) {
            delete m_widget;
            m_widget = 0;
        }
        Core::EditorManager::instance()->setParent(0);
    }

    QString displayName() const { return tr("Analyze"); }
    QIcon icon() const { return QIcon(":/images/analyzer_mode.png"); }
    int priority() const { return Constants::P_MODE_ANALYZE; }
    QWidget *widget() { return m_widget; }
    QString id() const { return QLatin1String(Constants::MODE_ANALYZE); }
    QString type() const { return Core::Constants::MODE_EDIT_TYPE; }
    Core::Context context() const
    {
        return Core::Context(Core::Constants::C_EDITORMANAGER, Constants::C_ANALYZEMODE,
                             Core::Constants::C_NAVIGATION_PANE);
    }
    QString contextHelpId() const { return QString(); }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QWidget *m_widget;
};

}

using Analyzer::Internal::AnalyzerRunControl;

class ANALYZER_EXPORT AnalyzerManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalyzerManager(QObject *parent = 0);
    ~AnalyzerManager();

    static AnalyzerManager *instance();

    bool isInitialized() const;
    void shutdown();

    /**
     * Register a tool and initialize it.
     */
    void addTool(Analyzer::IAnalyzerTool *tool);
    IAnalyzerTool *currentTool() const;
    QList<IAnalyzerTool *> tools() const;

    // dockwidgets are registered to the main window
    QDockWidget *createDockWidget(IAnalyzerTool *tool, const QString &title, QWidget *widget,
                                  Qt::DockWidgetArea area = Qt::TopDockWidgetArea);

    /**
     * Add the given @p widget into this mode's toolbar.
     *
     * It will be shown whenever this tool is selected by the user.
     *
     * @Note The manager will take ownership of @p widget.
     */
    void setToolbar(Analyzer::IAnalyzerTool *tool, QWidget *widget);

    Utils::FancyMainWindow *mainWindow() const;

    void selectTool(IAnalyzerTool *tool);

private slots:
    void startTool();
    void stopTool();
    void handleToolFinished();
    void toolSelected(int);
    void toolSelected(QAction *);
    void modeChanged(Core::IMode *mode);
    void runControlCreated(AnalyzerRunControl *);
    void resetLayout();
    void saveToolSettings(IAnalyzerTool *tool);
    void loadToolSettings(IAnalyzerTool *tool);
    void updateRunActions();

private:
    class AnalyzerManagerPrivate;
    friend class AnalyzerManagerPrivate;
    AnalyzerManagerPrivate *const d;

    static AnalyzerManager *m_instance;
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
