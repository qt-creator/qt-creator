// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <QStandardItemModel>

#include <functional>

namespace VcsBase {

enum CheckMode
{
    Unchecked,
    Checked,
    Uncheckable
};

class VCSBASE_EXPORT SubmitFileModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit SubmitFileModel(QObject *parent = nullptr);

    const QString &repositoryRoot() const;
    void setRepositoryRoot(const QString &repoRoot);

    // Convenience to create and add rows containing a file plus status text.
    QList<QStandardItem *> addFile(const QString &fileName, const QString &status = QString(),
                                   CheckMode checkMode = Checked, const QVariant &data = QVariant());

    QString state(int row) const;
    QString file(int row) const;
    bool isCheckable(int row) const;
    bool checked(int row) const;
    void setChecked(int row, bool check);
    void setAllChecked(bool check);
    QVariant extraData(int row) const;

    bool hasCheckedFiles() const;

    // Filter for entries contained in the filter list. Returns the
    // number of deleted entries.
    unsigned int filterFiles(const QStringList &filter);

    virtual void updateSelections(SubmitFileModel *source);

    enum FileStatusHint
    {
        FileStatusUnknown,
        FileAdded,
        FileModified,
        FileDeleted,
        FileRenamed,
        FileUnmerged
    };

    // Function that converts(qualifies) a QString/QVariant pair to FileStatusHint
    //     1st arg is the file status string as passed to addFile()
    //     2nd arg is the file extra data as passed to addFile()
    typedef std::function<FileStatusHint (const QString &, const QVariant &)>
            FileStatusQualifier;

    const FileStatusQualifier &fileStatusQualifier() const;
    void setFileStatusQualifier(FileStatusQualifier &&func);

private:
    QString m_repositoryRoot;
    FileStatusQualifier m_fileStatusQualifier;
};

} // namespace VcsBase
