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

#ifndef SELECTABLEFILESMODEL_H
#define SELECTABLEFILESMODEL_H

#include <QAbstractItemModel>
#include <QSet>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QDialog>
#include <QTreeView>
#include <QLabel>
#include "projectexplorer_export.h"

#include <utils/fileutils.h>

namespace Utils { class PathChooser; }

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Tree
{
public:
    ~Tree();

    QString name;
    Qt::CheckState checked;
    bool isDir;
    QList<Tree *> childDirectories;
    QList<Tree *> files;
    QList<Tree *> visibleFiles;
    QIcon icon;
    Utils::FileName fullPath;
    Tree *parent;
};

struct Glob
{
    enum Mode { EXACT, ENDSWITH, REGEXP };
    Mode mode;
    QString matchString;
    mutable QRegExp matchRegexp;

    bool isMatch(const QString &text) const
    {
        if (mode == Glob::EXACT) {
            if (text == matchString)
                return true;
        } else if (mode == Glob::ENDSWITH) {
            if (text.endsWith(matchString))
                return true;
        } else if (mode == Glob::REGEXP) {
            if (matchRegexp.exactMatch(text))
                return true;
        }
        return false;
    }
};

class PROJECTEXPLORER_EXPORT SelectableFilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SelectableFilesModel(QObject *parent);
    ~SelectableFilesModel();

    void setInitialMarkedFiles(const Utils::FileNameList &files);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    Utils::FileNameList selectedFiles() const;
    Utils::FileNameList selectedPaths() const;
    Utils::FileNameList preservedFiles() const;

    void startParsing(const Utils::FileName &baseDir);
    void cancel();
    void applyFilter(const QString &selectFilesfilter, const QString &hideFilesfilter);

    void selectAllFiles();

signals:
    void parsingFinished();
    void parsingProgress(const Utils::FileName &fileName);

private slots:
    void buildTreeFinished();

private:
    QList<Glob> parseFilter(const QString &filter);
    Qt::CheckState applyFilter(const QModelIndex &index);
    bool filter(Tree *t);
    void run(QFutureInterface<void> &fi);
    void collectFiles(Tree *root, Utils::FileNameList *result) const;
    void collectPaths(Tree *root, Utils::FileNameList *result) const;
    void buildTree(const Utils::FileName &baseDir, Tree *tree, QFutureInterface<void> &fi, int symlinkDepth);
    void propagateUp(const QModelIndex &index);
    void propagateDown(const QModelIndex &index);
    void selectAllFiles(Tree *root);

    Tree *m_root = 0;

    // Used in the future thread need to all not used after calling startParsing
    Utils::FileName m_baseDir;
    QSet<Utils::FileName> m_files;
    QSet<Utils::FileName> m_outOfBaseDirFiles;
    QFutureWatcher<void> m_watcher;
    Tree *m_rootForFuture = 0;
    int m_futureCount = 0;
    bool m_allFiles = true;

    QList<Glob> m_hideFilesFilter;
    QList<Glob> m_showFilesFilter;
};

class PROJECTEXPLORER_EXPORT SelectableFilesDialogEditFiles : public QDialog
{
    Q_OBJECT

public:
    SelectableFilesDialogEditFiles(const Utils::FileName &path, const Utils::FileNameList &files,
                                   QWidget *parent);
    ~SelectableFilesDialogEditFiles();
    Utils::FileNameList selectedFiles() const;

    void setAddFileFilter(const QString &filter);

private slots:
    void applyFilter();
    void parsingProgress(const QString &fileName);
    void parsingFinished();

protected:
    void smartExpand(const QModelIndex &index);
    void createShowFileFilterControls(QVBoxLayout *layout);
    void createHideFileFilterControls(QVBoxLayout *layout);
    void createApplyButton(QVBoxLayout *layout);

    SelectableFilesModel *m_selectableFilesModel;

    QLabel *m_hideFilesFilterLabel;
    QLineEdit *m_hideFilesfilterLineEdit;

    QLabel *m_showFilesFilterLabel;
    QLineEdit *m_showFilesfilterLineEdit;

    QPushButton *m_applyFilterButton;

    QTreeView *m_view;
    QLabel *m_preservedFiles;
    QLabel *m_progressLabel;
};

class SelectableFilesDialogAddDirectory : public SelectableFilesDialogEditFiles
{
    Q_OBJECT

public:
    SelectableFilesDialogAddDirectory(const Utils::FileName &path, const Utils::FileNameList &files,
                                      QWidget *parent);

private slots:
    void validityOfDirectoryChanged(bool validState);
    void parsingFinished();
    void startParsing();

private:
    Utils::PathChooser *m_pathChooser;
    QLabel *m_sourceDirectoryLabel;
    QPushButton *m_startParsingButton;

    void setWidgetsEnabled(bool enabled);
    void createPathChooser(QVBoxLayout *layout, const Utils::FileName &path);
};

} // namespace ProjectExplorer

#endif // SELECTABLEFILESMODEL_H

