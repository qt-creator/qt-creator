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

#ifndef IANALYZEROUTPUTPANEADAPTER_H
#define IANALYZEROUTPUTPANEADAPTER_H

#include "analyzerbase_global.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QAbstractItemModel;
class QWidget;
QT_END_NAMESPACE

namespace Analyzer {

class ANALYZER_EXPORT IAnalyzerOutputPaneAdapter : public QObject
{
    Q_OBJECT

public:
    explicit IAnalyzerOutputPaneAdapter(QObject *parent = 0);

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

} // namespace Analyzer

#endif // IANALYZEROUTPUTPANEADAPTER_H
