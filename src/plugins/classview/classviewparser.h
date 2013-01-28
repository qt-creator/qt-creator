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

#ifndef CLASSVIEWPARSER_H
#define CLASSVIEWPARSER_H

#include <QObject>

#include "classviewparsertreeitem.h"

#include <CPlusPlusForwardDeclarations.h>
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/CppDocument.h>

// might be changed to forward declaration - is not done to be less dependent
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <QList>
#include <QSharedPointer>
#include <QStandardItem>

namespace ClassView {
namespace Internal {

class ParserPrivate;

/*!
   \class Parser
   \brief Parse cpp information. Multithreading is supported.
 */

class Parser : public QObject
{
    Q_OBJECT

public:
    /*!
       \brief Constructor
     */
    explicit Parser(QObject *parent = 0);
    ~Parser();

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item which has to be checked
     */
    bool canFetchMore(QStandardItem *item) const;

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item which will be populated (if needed)
       \param skipRoot Skip root item
     */
    void fetchMore(QStandardItem *item, bool skipRoot = false) const;

signals:
    //! File list is changed
    void filesAreRemoved();

    /*!
       \brief Signal about a tree data update
     */
    void treeDataUpdate(QSharedPointer<QStandardItem> result);

    /*!
       \brief Signal that internal data
       \sa resetData, resetDataToCurrentState
     */
    void resetDataDone();

public slots:
    /*!
       \brief Request to clear full internal stored data.
     */
    void clearCacheAll();

    /*!
       \brief Request to clear internal stored data, it has to be regenerated on the next request.
     */
    void clearCache();

    /*!
       \brief Request to emit a signal with the current tree state
     */
    void requestCurrentState();

    /*!
       \brief Set file list for the parsing, files outside of this list will not be in any tree.
       \param fileList Files which must be allowed for the parsing
     */
    void setFileList(const QStringList &fileList);

    /*!
       \brief Remove some files from the file list for the parsing.
       \param fileList Files which must be removed from the parsing
     */
    void removeFiles(const QStringList &fileList);

    /*!
       \brief Fully reset internal state
       \param snapshot Code parser snapshot
     */
    void resetData(const CPlusPlus::Snapshot &snapshot);

    /*!
       \brief Fully reset internal state - to the current state
       \sa resetData
     */
    void resetDataToCurrentState();

    /*!
       \brief Parse document if it is in the project files and add a tree to the internal storage
       \param doc Document which has to be parsed
     */
    void parseDocument(const CPlusPlus::Document::Ptr &doc);

    /*!
       \brief Switch to flat mode (without subprojects)
       \param flat True to enable flat mode, false to disable
     */
    void setFlatMode(bool flat);

protected slots:
    /*!
       \brief Internal data is changed, regenerate the tree
       \sa resetDataDone
     */
    void onResetDataDone();

protected:
    /*!
       \brief Parse one particular project and add result to the tree item
       \param item Item where parsed project has to be stored
       \param fileList Files
       \param projectId Project id, is needed for prj cache
     */
    void addProject(const ParserTreeItem::Ptr &item, const QStringList &fileList,
                    const QString &projectId = QString());

    /*!
       \brief Parse one particular symbol and add result to the tree item (as a parent)
       \param item Item where parsed symbol has to be stored
       \param symbol Symbol which has to be used as a source
     */
    void addSymbol(const ParserTreeItem::Ptr &item, const CPlusPlus::Symbol *symbol);

    /*!
       \brief Parse document if it is in the project files and add a tree to the internal storage.
              Update internal cached tree for this document.
       \param doc Document which has to be parsed
       \return A tree
       \sa parseDocument
     */
    ParserTreeItem::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc);

    /*!
       \brief Get from the cache or parse document if it is in the project files
              and add a tree to the internal storage
       \param doc Document which has to be parsed
       \return A tree
       \sa parseDocument
     */
    ParserTreeItem::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc);

    /*!
       \brief Parse project and add a tree to the internal storage.
              Update internal cached tree for this project.
       \param fileList Documents which has to be added to the project
       \param projectId Project id
       \return A tree
     */
    ParserTreeItem::Ptr getParseProjectTree(const QStringList &fileList, const QString &projectId);

    /*!
       \brief Get from the cache (if valid) or parse project and add a tree to the internal storage.
              Update internal cached tree for this project.
       \param fileList Documents which has to be added to the project
       \param projectId Project id
       \return A tree
     */
    ParserTreeItem::Ptr getCachedOrParseProjectTree(const QStringList &fileList,
                                                    const QString &projectId);

    /*!
       \brief Send a current tree to listeners
     */
    void emitCurrentTree();

    /*!
       \brief Parse the class and produce a new tree
       \sa addProject
     */
    ParserTreeItem::ConstPtr parse();

    /*!
       \brief Find internal node for the specified UI item
       \param item Item which has to be found
       \param skipRoot Skip root item
       \return Found internal node
     */
    ParserTreeItem::ConstPtr findItemByRoot(const QStandardItem *item, bool skipRoot = false) const;

    /*!
       \brief Generate projects like Project Explorer
       \param item Item
       \param node Root node
       \return List of projects which were added to the item
     */
    QStringList addProjectNode(const ParserTreeItem::Ptr &item,
                               const ProjectExplorer::ProjectNode *node);

    /*!
       \brief Generate project node file list
       \param node Root node
     */
    QStringList projectNodeFileList(const ProjectExplorer::FolderNode *node) const;

    /*!
       \brief Get the current project list
       \return Project list
     */
    QList<ProjectExplorer::Project *> getProjectList() const;

    /*!
       \brief Create flat tree from different projects
       \param projectList List of projects
       \return Flat tree
     */
    ParserTreeItem::Ptr createFlatTree(const QStringList &projectList);

private:
    //! Private class data pointer
    ParserPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWPARSER_H
