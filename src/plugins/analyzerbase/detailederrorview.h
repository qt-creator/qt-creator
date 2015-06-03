/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DETAILEDERRORVIEW_H
#define DETAILEDERRORVIEW_H

#include "analyzerbase_global.h"

#include <QListView>
#include <QStyledItemDelegate>

namespace Analyzer {

// Provides the details widget for the DetailedErrorView
class ANALYZER_EXPORT DetailedErrorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    struct SummaryLineInfo {
        QString errorText;
        QString errorLocation;
    };

public:
    /// This delegate can only work on one view at a time, parent. parent will also be the parent
    /// in the QObject parent-child system.
    explicit DetailedErrorDelegate(QListView *parent);

    virtual SummaryLineInfo summaryInfo(const QModelIndex &index) const = 0;
    void copyToClipboard();

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

public slots:
    void onCurrentSelectionChanged(const QModelIndex &now, const QModelIndex &previous);
    void onViewResized();
    void onLayoutChanged();

private slots:
    void onVerticalScroll();

protected:
    void openLinkInEditor(const QString &link);
    mutable QPersistentModelIndex m_detailsIndex;

private:
    // the constness of this function is a necessary lie because it is called from paint() const.
    virtual QWidget *createDetailsWidget(const QFont &font, const QModelIndex &errorIndex,
                                         QWidget *parent) const = 0;
    virtual QString textualRepresentation() const = 0;

    static const int s_itemMargin = 2;
    mutable QWidget *m_detailsWidget;
    mutable int m_detailsWidgetHeight;
};

// A QListView that displays additional details for the currently selected item
class ANALYZER_EXPORT DetailedErrorView : public QListView
{
    Q_OBJECT

public:
    DetailedErrorView(QWidget *parent = 0);
    ~DetailedErrorView();

    // Reimplemented to connect delegate to connection model after it has
    // been set by superclass implementation.
    void setModel(QAbstractItemModel *model);

    void setItemDelegate(QAbstractItemDelegate *delegate); // Takes ownership

    void goNext();
    void goBack();

signals:
    void resized();

protected:
    void updateGeometries();
    void resizeEvent(QResizeEvent *e);

private:
    void contextMenuEvent(QContextMenuEvent *e) override;

    int currentRow() const;
    void setCurrentRow(int row);
    int rowCount() const;

    QList<QAction *> commonActions() const;
    virtual QList<QAction *> customActions() const;

    QAction *m_copyAction;
};

} // namespace Analyzer

#endif // DETAILEDERRORVIEW_H
