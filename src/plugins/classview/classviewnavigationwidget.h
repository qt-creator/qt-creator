/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CLASSVIEWNAVIGATIONWIDGET_H
#define CLASSVIEWNAVIGATIONWIDGET_H

#include <QList>
#include <QSharedPointer>

#include <QStandardItem>
#include <QToolButton>
#include <QWidget>

namespace ClassView {
namespace Internal {

class NavigationWidgetPrivate;

/*!
   \class NavigationWidget
   \brief A widget for the class view tree
 */

class NavigationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationWidget(QWidget *parent = 0);
    ~NavigationWidget();

    /*!
       \brief Create QToolbuttons for Navigation Pane Widget
       \return List with created QToolButtons
       \sa NavigationWidgetFactory::createWidget
     */
    QList<QToolButton *> createToolButtons();

    /*!
       \brief Get flat mode state
       \return Flat mode state
     */
    bool flatMode() const;

    /*!
       \brief Set flat mode state
       \param flatMode Flat mode state
     */
    void setFlatMode(bool flatMode);

signals:
    /*!
       \brief Widget visibility is changed
       \param visibility true is plugin becames visible, false otherwise
     */
    void visibilityChanged(bool visibility);

    /*!
       \brief Signal to request to go to location
       \param name File which has to be open
       \param line Line
       \param column Column
       \sa Manager::gotoLocation
     */
    void requestGotoLocation(const QString &name, int line, int column);

    /*!
       \brief Signal to request to go to any of location in the list
       \param locations Symbol locations
       \sa Manager::gotoLocations
     */
    void requestGotoLocations(const QList<QVariant> &locations);

    /*!
       \brief Signal that the widget wants to receive the latest tree info
       \sa Manager::onRequestTreeDataUpdate
     */
    void requestTreeDataUpdate();

public slots:
    /*!
       \brief Item is activated in the tree view
       \param index Item index
     */
    void onItemActivated(const QModelIndex &index);

    /*!
       \brief Receive a new data for the tree
       \param Result - root item
     */
    void onDataUpdate(QSharedPointer<QStandardItem> result);

    /*!
       \brief Full projects' mode button has been toggled
       \param state Full projects' mode
     */
    void onFullProjectsModeToggled(bool state);

protected:
    /*!
       \brief Fetch data for expanded items - to be sure that content will exist
       \param item
       \param target
     */
    void fetchExpandedItems(QStandardItem *item, const QStandardItem *target) const;

    //! implements QWidget::hideEvent
    void hideEvent(QHideEvent *event);

    //! implements QWidget::showEvent
    void showEvent(QShowEvent *event);

private:
    //! Private class data pointer
    NavigationWidgetPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWNAVIGATIONWIDGET_H
