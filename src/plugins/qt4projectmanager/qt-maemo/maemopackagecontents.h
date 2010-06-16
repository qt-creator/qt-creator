/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMOPACKAGECONTENTS_H
#define MAEMOPACKAGECONTENTS_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class ProFile;
struct ProFileOption;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class MaemoPackageCreationStep;
class ProFileReader;

class MaemoPackageContents : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Deployable
    {
        Deployable(const QString &localFilePath, const QString &remoteDir)
            : localFilePath(localFilePath), remoteDir(remoteDir) {}

        bool operator==(const Deployable &other) const
        {
            return localFilePath == other.localFilePath
                    && remoteDir == other.remoteDir;
        }

        QString localFilePath;
        QString remoteDir;
    };

    MaemoPackageContents(MaemoPackageCreationStep *packageStep);
    ~MaemoPackageContents();

    bool init();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    Deployable deployableAt(int row) const;
    bool addDeployable(const Deployable &deployable, QString *error);
    bool removeDeployableAt(int row, QString *error);
    bool isModified() const { return m_modified; }
    void setUnModified() { m_modified = false; }
    QString remoteExecutableFilePath() const;

private:
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);

    bool buildModel() const;
    void resetProFileContents() const;
    bool readProFileContents(QString *error) const;
    bool writeProFileContents(QString *error) const;

    QString cleanPath(const QString &relFileName) const;

    QString findInstallsElem(const Deployable &deployable) const;
    void addFileToProFile(const QString &var, const QString &absFilePath);
    void addValueToProFile(const QString &var, const QString &value) const;
    bool removeFileFromProFile(const QString &var, const QString &absFilePath);
    bool removeValueFromProFile(const QString &var, const QString &value);

    enum ParseType { ParseFromFile, ParseFromLines };
    void parseProFile(ParseType type) const;

    const MaemoPackageCreationStep * const m_packageStep;
    QScopedPointer<ProFileOption> m_proFileOption;
    QScopedPointer<ProFileReader> m_proFileReader;
    mutable QList<Deployable> m_deployables;
    mutable bool m_modified;
    mutable ProFile *m_proFile;
    mutable QStringList m_proFileLines; // TODO: FS watcher
    mutable QString m_proFileName;
    mutable QDir m_proDir;
};

} // namespace Qt4ProjectManager
} // namespace Internal

#endif // MAEMOPACKAGECONTENTS_H
