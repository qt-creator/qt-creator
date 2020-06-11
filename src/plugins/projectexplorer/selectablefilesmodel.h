/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QAbstractItemModel>
#include <QDialog>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>
#include <QTreeView>

namespace Utils {
class FancyLineEdit;
class PathChooser;
}

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Tree
{
public:
    virtual ~Tree()
    {
        qDeleteAll(childDirectories);
        qDeleteAll(files);
    }

    QString name;
    Qt::CheckState checked = Qt::Unchecked;
    bool isDir = false;
    QList<Tree *> childDirectories;
    QList<Tree *> files;
    QList<Tree *> visibleFiles;
    QIcon icon;
    Utils::FilePath fullPath;
    Tree *parent = nullptr;
};

class Glob
{
public:
    enum Mode { EXACT, ENDSWITH, REGEXP };
    Mode mode;
    QString matchString;
    QRegularExpression matchRegexp;

    bool isMatch(const QString &text) const
    {
        if (mode == Glob::EXACT) {
            if (text == matchString)
                return true;
        } else if (mode == Glob::ENDSWITH) {
            if (text.endsWith(matchString))
                return true;
        } else if (mode == Glob::REGEXP) {
            if (matchRegexp.match(text).hasMatch())
                return true;
        }
        return false;
    }

    bool operator == (const Glob &other) const
    {
        return (mode == other.mode)
                && (matchString == other.matchString)
                && (matchRegexp == other.matchRegexp);
    }
};

class PROJECTEXPLORER_EXPORT SelectableFilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SelectableFilesModel(QObject *parent);
    ~SelectableFilesModel() override;

    void setInitialMarkedFiles(const Utils::FilePaths &files);

    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Utils::FilePaths selectedFiles() const;
    Utils::FilePaths selectedPaths() const;
    Utils::FilePaths preservedFiles() const;

    bool hasCheckedFiles() const;

    void applyFilter(const QString &selectFilesfilter, const QString &hideFilesfilter);

    void selectAllFiles();

    enum class FilterState { HIDDEN, SHOWN, CHECKED };
    FilterState filter(Tree *t);

signals:
    void checkedFilesChanged();

protected:
    void propagateUp(const QModelIndex &index);
    void propagateDown(const QModelIndex &idx);

private:
    QList<Glob> parseFilter(const QString &filter);
    Qt::CheckState applyFilter(const QModelIndex &idx);
    void collectFiles(Tree *root, Utils::FilePaths *result) const;
    void collectPaths(Tree *root, Utils::FilePaths *result) const;
    void selectAllFiles(Tree *root);

protected:
    bool m_allFiles = true;
    QSet<Utils::FilePath> m_outOfBaseDirFiles;
    QSet<Utils::FilePath> m_files;
    Tree *m_root = nullptr;

private:
    QList<Glob> m_hideFilesFilter;
    QList<Glob> m_selectFilesFilter;
};

class PROJECTEXPLORER_EXPORT SelectableFilesFromDirModel : public SelectableFilesModel
{
    Q_OBJECT

public:
    SelectableFilesFromDirModel(QObject *parent);
    ~SelectableFilesFromDirModel() override;

    void startParsing(const Utils::FilePath &baseDir);
    void cancel();

signals:
    void parsingFinished();
    void parsingProgress(const Utils::FilePath &fileName);

private:
    void buildTree(const Utils::FilePath &baseDir,
                   Tree *tree,
                   QFutureInterface<void> &fi,
                   int symlinkDepth);
    void run(QFutureInterface<void> &fi);
    void buildTreeFinished();

    // Used in the future thread need to all not used after calling startParsing
    Utils::FilePath m_baseDir;
    QFutureWatcher<void> m_watcher;
    Tree *m_rootForFuture = nullptr;
    int m_futureCount = 0;
};

class PROJECTEXPLORER_EXPORT SelectableFilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SelectableFilesWidget(QWidget *parent = nullptr);
    SelectableFilesWidget(const Utils::FilePath &path, const Utils::FilePaths &files,
                          QWidget *parent = nullptr);

    void setAddFileFilter(const QString &filter);
    void setBaseDirEditable(bool edit);

    Utils::FilePaths selectedFiles() const;
    Utils::FilePaths selectedPaths() const;

    bool hasFilesSelected() const;

    void resetModel(const Utils::FilePath &path, const Utils::FilePaths &files);
    void cancelParsing();

    void enableFilterHistoryCompletion(const QString &keyPrefix);

signals:
    void selectedFilesChanged();

private:
    void enableWidgets(bool enabled);
    void applyFilter();
    void baseDirectoryChanged(bool validState);

    void startParsing(const Utils::FilePath &baseDir);
    void parsingProgress(const Utils::FilePath &fileName);
    void parsingFinished();

    void smartExpand(const QModelIndex &idx);

    SelectableFilesFromDirModel *m_model = nullptr;

    Utils::PathChooser *m_baseDirChooser;
    QLabel *m_baseDirLabel;
    QPushButton *m_startParsingButton;

    QLabel *m_selectFilesFilterLabel;
    Utils::FancyLineEdit *m_selectFilesFilterEdit;

    QLabel *m_hideFilesFilterLabel;
    Utils::FancyLineEdit *m_hideFilesFilterEdit;

    QPushButton *m_applyFiltersButton;

    QTreeView *m_view;

    QLabel *m_preservedFilesLabel;

    QLabel *m_progressLabel;
    bool m_filteringScheduled = false;
};

class PROJECTEXPLORER_EXPORT SelectableFilesDialogEditFiles : public QDialog
{
    Q_OBJECT

public:
    SelectableFilesDialogEditFiles(const Utils::FilePath &path, const Utils::FilePaths &files,
                                   QWidget *parent);
    Utils::FilePaths selectedFiles() const;

    void setAddFileFilter(const QString &filter) { m_filesWidget->setAddFileFilter(filter); }

protected:
    SelectableFilesWidget *m_filesWidget;
};

class SelectableFilesDialogAddDirectory : public SelectableFilesDialogEditFiles
{
    Q_OBJECT

public:
    SelectableFilesDialogAddDirectory(const Utils::FilePath &path, const Utils::FilePaths &files,
                                      QWidget *parent);
};

} // namespace ProjectExplorer
