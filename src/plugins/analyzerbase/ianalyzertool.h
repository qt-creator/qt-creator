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

#ifndef IANALYZERTOOL_H
#define IANALYZERTOOL_H

#include "analyzerbase_global.h"

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QAbstractItemView)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)

namespace ProjectExplorer {
class RunConfiguration;
}

namespace ExtensionSystem {
class IPlugin;
}

namespace Analyzer {
class IAnalyzerEngine;

class ANALYZER_EXPORT IAnalyzerOutputPaneAdapter : public QObject
{
    Q_OBJECT
public:
    explicit IAnalyzerOutputPaneAdapter(QObject *parent = 0);
    virtual ~IAnalyzerOutputPaneAdapter();

    virtual QWidget *toolBarWidget() = 0;
    virtual QWidget *paneWidget() = 0;
    virtual void clearContents() = 0;
    virtual void setFocus() = 0;
    virtual bool hasFocus() const = 0;
    virtual bool canFocus() const = 0;
    virtual bool canNavigate() const = 0;
    virtual bool canNext() const = 0;
    virtual bool canPrevious() const = 0;
    virtual void goToNext() = 0;
    virtual void goToPrev() = 0;

signals:
    void popup(bool withFocus);
    void navigationStatusChanged();
};

class ANALYZER_EXPORT ListItemViewOutputPaneAdapter : public IAnalyzerOutputPaneAdapter
{
    Q_OBJECT
public:
    explicit ListItemViewOutputPaneAdapter(QObject *parent = 0);

    virtual QWidget *paneWidget();
    virtual void setFocus();
    virtual bool hasFocus() const;
    virtual bool canFocus() const;
    virtual bool canNavigate() const;
    virtual bool canNext() const;
    virtual bool canPrevious() const;
    virtual void goToNext();
    virtual void goToPrev();

    bool showOnRowsInserted() const;
    void setShowOnRowsInserted(bool v);

protected:
    int currentRow() const;
    void setCurrentRow(int);
    int rowCount() const;
    void connectNavigationSignals(QAbstractItemModel *);
    virtual QAbstractItemView *createItemView() = 0;

private slots:
    void slotRowsInserted();

private:
    QAbstractItemView *m_listView;
    bool m_showOnRowsInserted;
};

class ANALYZER_EXPORT IAnalyzerTool : public QObject
{
    Q_OBJECT
public:
    explicit IAnalyzerTool(QObject *parent = 0);

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    /**
     * The mode in which this tool should be run preferrably
     *
     * e.g. memcheck requires debug symbols, hence DebugMode is prefferred.
     * otoh callgrind should look at optimized code, hence ReleaseMode.
     */
    enum ToolMode {
        DebugMode,
        ReleaseMode,
        AnyMode
    };
    virtual ToolMode mode() const = 0;

    QString modeString();

    virtual void initialize(ExtensionSystem::IPlugin *plugin) = 0;

    virtual IAnalyzerOutputPaneAdapter *outputPaneAdapter();

    virtual IAnalyzerEngine *createEngine(ProjectExplorer::RunConfiguration *runConfiguration) = 0;
};

} // namespace Analyzer

#endif // IANALYZERTOOL_H
