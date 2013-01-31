/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
** Contact: http://www.qt-project.org/legal
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

#ifndef CLASSVIEWMANAGER_H
#define CLASSVIEWMANAGER_H

#include <QObject>
#include <QSharedPointer>
#include <QStandardItem>

#include <cplusplus/CppDocument.h>

namespace ClassView {
namespace Internal {

class ManagerPrivate;

/*!
   \class Manager
   \brief Class View manager

   Class View Manager. Interacts with other Qt Creator plugins - this is a proxy between them and
   parser.
   \a Parser is moved to a separate thread and is connected to \a Manager by signal/slots.
   Manager's signals starting with 'request' are for Parser.
 */

class Manager : public QObject
{
    Q_OBJECT

public:
    /*!
       \brief Creates shared instance
       \param parent Parent object
     */
    explicit Manager(QObject *parent = 0);

    virtual ~Manager();

    //! Get an instance of Manager
    static Manager *instance();

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item with has to be checked
     */
    bool canFetchMore(QStandardItem *item) const;

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item with has to be checked
       \param skipRoot Skip root item (is needed for the manual update, call not from model)
     */
    void fetchMore(QStandardItem *item, bool skipRoot = false);

signals:
    /*!
       \brief Internal Manager state is changed.
       \param state true if Manager is enabled, false otherwise
       \sa setState, state
     */
    void stateChanged(bool state);

    /*!
       \brief Signal about a tree data update (to tree view).
       \param result Item with the current tree
     */
    void treeDataUpdate(QSharedPointer<QStandardItem> result);

    /*!
       \brief Signal that a request for sending tree view has to be handled by listeners (parser).
       \sa onRequestTreeDataUpdate
     */
    void requestTreeDataUpdate();

    /*!
       \brief Signal that document is updated (and has to be reparsed)
       \param doc Updated document
       \sa onDocumentUpdated
     */
    void requestDocumentUpdated(CPlusPlus::Document::Ptr doc);

    /*!
       \brief Signal that parser has to reset its internal state to the current (from code manager).
     */
    void requestResetCurrentState();

    /*!
       \brief Request to clear a cache by parser.
     */
    void requestClearCache();

    /*!
       \brief Request to clear a full cache by parser.
     */
    void requestClearCacheAll();

    /*!
       \brief Request to set the flat mode (without subprojects)
       \param flat True to enable flat mode, false to disable
     */
    void requestSetFlatMode(bool flat);

public slots:
    /*!
       \brief Open text editor for file \a fileName on line \a lineNumber and column \a column.
       \param fileName File which has to be open
       \param line Line number, 1-based
       \param column Column, 1-based
     */
    void gotoLocation(const QString &fileName, int line = 0, int column = 0);

    /*!
       \brief Open text editor for any of location in the list (correctly)
       \param locations Symbol locations
       \sa Manager::gotoLocations
     */
    void gotoLocations(const QList<QVariant> &locations);

    /*!
       \brief If somebody wants to receive the latest tree info, if a parsing is enabled then
              a signal \a requestTreeDataUpdate will be emitted.
       \sa requestTreeDataUpdate, NavigationWidget::requestDataUpdate
     */
    void onRequestTreeDataUpdate();

    /*!
       \brief Switch to flat mode (without subprojects)
       \param flat True to enable flat mode, false to disable
     */
    void setFlatMode(bool flat);

protected slots:
    /*!
       \brief Widget factory creates a widget, handle this situation.
       \sa setState, state
     */
    void onWidgetIsCreated();

    /*!
       \brief Widget visibility is changed
       \param visibility Visibility (for just 1 navi pane widget, there might be a lot of them)
       \sa setState, state
     */
    void onWidgetVisibilityIsChanged(bool visibility);

    /*!
       \brief Reacts to the state changed signal, e.g. request currect code snapshot if needed etc.
       \param state Current Manager state
       \sa setState, state, stateChanged
     */
    void onStateChanged(bool state);

    /*!
       \brief Project list is changed (navigation pane visibility might be needed to update).
     */
    void onProjectListChanged();

    /*!
       \brief This slot should called when the code model manager state is changed for \a doc.
              It will emit a signal \a documentUpdated if
       \param doc Updated document.
       \sa documentUpdated
     */
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);

    /*!
       \brief Progress manager started a task. Do what is needed if it is a parse task.
       \param type Task index, should be CppTools::Constants::TASK_INDEX for us
       \sa CppTools::Constants::TASK_INDEX
     */
    void onTaskStarted(const QString &type);

    /*!
       \brief Progress manager finished all task with specified type.
              Do what is needed if it is a parse task.
       \param type Task index, should be CppTools::Constants::TASK_INDEX for us
       \sa CppTools::Constants::TASK_INDEX
     */
    void onAllTasksFinished(const QString &type);

    /*!
       \brief New tree data update (has to be sent to a tree view).
       \param result Item with the current tree
     */
    void onTreeDataUpdate(QSharedPointer<QStandardItem> result);

protected:
    //! Perform an initialization
    void initialize();

    /*!
       \brief Get internal Manager state. If it is disabled, signals about parsing request has not
              to be emitted at all, if enabled - do parsing in the background even if navi pane
              is not visible.
       \return true if Manager is enabled, false otherwise
       \sa setState, stateChanged
     */
    inline bool state() const;

    /*!
       \brief Set internal Manager state.
       \param state true if Manager has to be enabled, false otherwise
       \sa state, stateChanged
     */
    void setState(bool state);

private:
    //! private class data pointer
    ManagerPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWMANAGER_H
