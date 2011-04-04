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

#ifndef CALLGRINDWIDGETHANDLER_H
#define CALLGRINDWIDGETHANDLER_H

#include <QObject>

#include "callgrindcostdelegate.h"

QT_BEGIN_NAMESPACE
class QToolButton;
class QAction;
class QLineEdit;
class QModelIndex;
class QSortFilterProxyModel;
class QTimer;
class QComboBox;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class DataModel;
class DataProxyModel;
class Function;
class ParseData;
class StackBrowser;
class CallModel;
}
}

namespace Callgrind {
namespace Internal {

class CallgrindEngine;
class Visualisation;
class CostView;

/**
 * Encapsulates all widgets required for a fully operational callgrind UI.
 * Prevents code duplication between QtC code and unit tests.
 */
class CallgrindWidgetHandler : public QObject
{
    Q_OBJECT

public:
    explicit CallgrindWidgetHandler(QWidget *parent);
    virtual ~CallgrindWidgetHandler();

    void populateActions(QLayout *layout);

    void setParseData(Valgrind::Callgrind::ParseData *data);

    Valgrind::Callgrind::DataModel *dataModel() const { return m_dataModel; }
    Valgrind::Callgrind::DataProxyModel *proxyModel() const { return m_dataProxy; }
    CostView *flatView() const { return m_flatView; }
    Visualisation *visualisation() const { return m_visualisation; }

    Valgrind::Callgrind::CallModel *callersModel() const { return m_callersModel; }
    CostView *callersView() const { return m_callersView; }
    Valgrind::Callgrind::CallModel *calleesModel() const { return m_calleesModel; }
    CostView *calleesView() const { return m_calleesView; }

signals:
    void dumpRequested();
    void resetRequested();
    void pauseToggled(bool checked);

    void functionSelected(const Valgrind::Callgrind::Function *);

    void costFormatChanged(Callgrind::Internal::CostDelegate::CostFormat format);
    void cycleDetectionEnabled(bool enabled);

public slots:
    void slotClear();
    void slotRequestDump();

    void selectFunction(const Valgrind::Callgrind::Function *);
    void setCostFormat(Callgrind::Internal::CostDelegate::CostFormat format);
    void enableCycleDetection(bool enabled);
    void setCostEvent(int index);

private slots:
    void updateFilterString();
    void updateCostFormat();

    void handleFilterProjectCosts();

    void slotGoToOverview();
    void stackBrowserChanged();

    void dataFunctionSelected(const QModelIndex &index);
    void calleeFunctionSelected(const QModelIndex &index);
    void callerFunctionSelected(const QModelIndex &index);
    void visualisationFunctionSelected(const Valgrind::Callgrind::Function *function);

private:
    void updateEventCombo();

    Valgrind::Callgrind::DataModel *m_dataModel;
    Valgrind::Callgrind::DataProxyModel *m_dataProxy;
    Valgrind::Callgrind::StackBrowser *m_stackBrowser;

    Valgrind::Callgrind::CallModel *m_callersModel;
    Valgrind::Callgrind::CallModel *m_calleesModel;

    CostView *m_flatView;
    CostView *m_callersView;
    CostView *m_calleesView;
    Visualisation *m_visualisation;

    // navigation
    QAction *m_goToOverview;
    QAction *m_goBack;
    QLineEdit *m_searchFilter;

    // cost formatting
    QAction *m_filterProjectCosts;
    QAction *m_costAbsolute;
    QAction *m_costRelative;
    QAction *m_costRelativeToParent;
    QAction *m_cycleDetection;
    QComboBox *m_eventCombo;

    QTimer *m_updateTimer;
};

} // namespace Internal
} // namespace Callgrind

#endif // CALLGRINDWIDGETHANDLER_H
