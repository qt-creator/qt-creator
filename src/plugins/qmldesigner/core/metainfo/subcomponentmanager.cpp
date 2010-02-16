/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "subcomponentmanager.h"
#include "metainfo.h"

#include <QDir>
#include <QMetaType>
#include <QUrl>
#include <QmlDomDocument>
#include <QmlEngine>
#include <QmlMetaType>
#include <QFileSystemWatcher>

enum { debug = false };

QT_BEGIN_NAMESPACE

// Allow usage of QFileInfo in hash / qSort

static bool operator<(const QFileInfo &file1, const QFileInfo &file2)
{
    return file1.filePath() < file2.filePath();
}

static uint qHash(const QFileInfo &fileInfo)
{
    return qHash(fileInfo.filePath());
}

QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

static const QString QMLFILEPATTERN = QString(QLatin1String("*.qml"));


class SubComponentManagerPrivate : QObject {
    Q_OBJECT
public:
    SubComponentManagerPrivate(MetaInfo metaInfo, SubComponentManager *q);

    void addImport(int pos, const QmlDomImport &import);
    void removeImport(int pos);
    void parseDirectories();

public slots:
    void parseDirectory(const QString &dirPath);
    void parseFile(const QString &filePath);

public:
    QList<QFileInfo> watchedFiles(const QDir &dirInfo);
    void unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier);
    void registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier, const QmlDomDocument &document);

    SubComponentManager *m_q;

    MetaInfo m_metaInfo;
    QmlEngine m_engine;

    QFileSystemWatcher m_watcher;

    QMultiHash<QFileInfo,QString> m_dirToQualifier;

    QUrl m_filePath;

    QList<QmlDomImport> m_imports;
};

SubComponentManagerPrivate::SubComponentManagerPrivate(MetaInfo metaInfo, SubComponentManager *q) :
        m_q(q),
        m_metaInfo(metaInfo)
{
    connect(&m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(parseDirectory(QString)));
    connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(parseFile(QString)));
}

void SubComponentManagerPrivate::addImport(int pos, const QmlDomImport &import)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << pos << import.uri();

    if (import.type() == QmlDomImport::File) {
        QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.uri()).toLocalFile());
        if (dirInfo.exists() && dirInfo.isDir()) {
            m_watcher.addPath(dirInfo.filePath());
            m_dirToQualifier.insertMulti(dirInfo, import.qualifier());
        }
    } else {
        // TODO: QmlDomImport::Library
    }

    m_imports.insert(pos, import);
}

void SubComponentManagerPrivate::removeImport(int pos)
{
    const QmlDomImport import = m_imports.takeAt(pos);

    if (import.type() == QmlDomImport::File) {
        QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.uri()).toLocalFile());

        m_dirToQualifier.remove(dirInfo, import.qualifier());

        if (!m_dirToQualifier.contains(dirInfo))
            m_watcher.removePath(dirInfo.filePath());

        foreach (const QFileInfo &monitoredFile, watchedFiles(dirInfo.filePath())) {
            if (!m_dirToQualifier.contains(dirInfo))
                m_watcher.removePath(monitoredFile.filePath());
            unregisterQmlFile(monitoredFile, import.qualifier());
        }
    } else {
            // TODO: QmlDomImport::Library
    }
}

void SubComponentManagerPrivate::parseDirectories()
{
    if (!m_filePath.isEmpty()) {
        const QString file = m_filePath.toLocalFile();
        QFileInfo dirInfo = QFileInfo(QFileInfo(file).path());
        if (dirInfo.exists() && dirInfo.isDir())
            parseDirectory(dirInfo.filePath());
    }

    foreach (const QmlDomImport &import, m_imports) {
        if (import.type() == QmlDomImport::File) {
            QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.uri()).toLocalFile());
            if (dirInfo.exists() && dirInfo.isDir()) {
                parseDirectory(dirInfo.filePath());
            }
        }
    }
}

void SubComponentManagerPrivate::parseDirectory(const QString &dirPath)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << dirPath;

    QDir dir(dirPath);

    dir.setNameFilters(QStringList(QMLFILEPATTERN));
    dir.setFilter(QDir::Files | QDir::Readable | QDir::CaseSensitive);

    QList<QFileInfo> monitoredList = watchedFiles(dir);
    QList<QFileInfo> newList;
    foreach (const QFileInfo &qmlFile, dir.entryInfoList()) {
        if (QFileInfo(m_filePath.toLocalFile()) == qmlFile) {
            // do not parse main file
            continue;
        }
        if (!qmlFile.fileName().at(0).isUpper()) {
            // Qml sub components must be upper case
            continue;
        }
        newList << qmlFile;
    }

    qSort(monitoredList);
    qSort(newList);

    if (debug)
        qDebug() << "monitored list " << monitoredList.size() << "new list " << newList.size();
    QList<QFileInfo>::const_iterator oldIter = monitoredList.constBegin();
    QList<QFileInfo>::const_iterator newIter = newList.constBegin();

    while (oldIter != monitoredList.constEnd() && newIter != newList.constEnd()) {
        const QFileInfo oldFileInfo = *oldIter;
        const QFileInfo newFileInfo = *newIter;
        if (oldFileInfo == newFileInfo) {
            ++oldIter;
            ++newIter;
            continue;
        }
        if (oldFileInfo < newFileInfo) {
            foreach (const QString &qualifier, m_dirToQualifier.value(dirPath))
                unregisterQmlFile(oldFileInfo, qualifier);
            m_watcher.removePath(oldFileInfo.filePath());
            ++oldIter;
            continue;
        }
        // oldFileInfo > newFileInfo
        parseFile(newFileInfo.filePath());
        m_watcher.addPath(oldFileInfo.filePath());
        ++newIter;
    }

    while (oldIter != monitoredList.constEnd()) {
        foreach (const QString &qualifier, m_dirToQualifier.value(dirPath))
            unregisterQmlFile(*oldIter, qualifier);
        m_watcher.removePath(oldIter->filePath());
        ++oldIter;
    }

    while (newIter != newList.constEnd()) {
        parseFile(newIter->filePath());
        if (debug)
            qDebug() << "m_watcher.addPath(" << newIter->filePath() << ')';
        m_watcher.addPath(newIter->filePath());
        ++newIter;
    }
}

void SubComponentManagerPrivate::parseFile(const QString &filePath)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QmlDomDocument document;
    if (!document.load(&m_engine, file.readAll(), QUrl::fromLocalFile(filePath))) {
        // TODO: Put the errors somewhere?
        qWarning() << "Could not load qml file " << filePath;
        return;
    }

    QString dir = QFileInfo(filePath).dir().path();
    Q_ASSERT(m_dirToQualifier.contains(dir));
    foreach (const QString &qualifier, m_dirToQualifier.values(dir)) {
        registerQmlFile(filePath, qualifier, document);
    }
}

QList<QFileInfo> SubComponentManagerPrivate::watchedFiles(const QDir &dirInfo)
{
    QList<QFileInfo> files;

    const QString dirPath = dirInfo.absolutePath();
    foreach (const QString &monitoredFile, m_watcher.files()) {
        QFileInfo fileInfo(monitoredFile);
        if (fileInfo.dir().absolutePath() == dirPath) {
            files.append(fileInfo);
        }
    }
    return files;
}

void SubComponentManagerPrivate::unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier)
{
    QString componentName = fileInfo.baseName();
    if (!qualifier.isEmpty())
        componentName = qualifier + '/' + componentName;

    if (m_metaInfo.hasNodeMetaInfo(componentName)) {
        NodeMetaInfo nodeInfo = m_metaInfo.nodeMetaInfo(componentName);
        m_metaInfo.removeNodeInfo(nodeInfo);
    }
}

void SubComponentManagerPrivate::registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier, const QmlDomDocument &document)
{
    QString componentName = fileInfo.baseName();
    if (!qualifier.isEmpty())
        componentName = qualifier + '/' + componentName;

    if (debug)
        qDebug() << "SubComponentManager" << __FUNCTION__ << componentName;

    if (m_metaInfo.hasNodeMetaInfo(componentName)) {
        NodeMetaInfo nodeInfo = m_metaInfo.nodeMetaInfo(componentName);
        m_metaInfo.removeNodeInfo(nodeInfo);
    }

    const QmlDomObject rootObject = document.rootObject();

    const QString baseType = document.rootObject().objectType();
    Q_ASSERT_X(!baseType.isEmpty(), Q_FUNC_INFO, "Type of root object cannot be empty");

    NodeMetaInfo nodeInfo(m_metaInfo);
    nodeInfo.setTypeName(componentName);
    nodeInfo.setQmlFile(fileInfo.filePath());

    // Add file components to the library
    nodeInfo.setCategory(tr("Qml Component"));
    nodeInfo.setIsVisibleToItemLibrary(true);

    m_metaInfo.addItemLibraryInfo(nodeInfo, componentName);
    m_metaInfo.addNodeInfo(nodeInfo, baseType);

    foreach (const QmlDomDynamicProperty &dynamicProperty, document.rootObject().dynamicProperties()) {
        Q_ASSERT(!dynamicProperty.propertyName().isEmpty());
        Q_ASSERT(!dynamicProperty.propertyTypeName().isEmpty());

        PropertyMetaInfo propertyMetaInfo;
        propertyMetaInfo.setName(dynamicProperty.propertyName());
        propertyMetaInfo.setType(dynamicProperty.propertyTypeName());
        propertyMetaInfo.setValid(true);
        propertyMetaInfo.setReadable(true);
        propertyMetaInfo.setWritable(true);

        QmlDomProperty defaultValue = dynamicProperty.defaultValue();
        if (defaultValue.value().isLiteral()) {
            QVariant defaultValueVariant(defaultValue.value().toLiteral().literal());
            defaultValueVariant.convert((QVariant::Type) dynamicProperty.propertyType());
            propertyMetaInfo.setDefaultValue(nodeInfo, defaultValueVariant);
        }

        nodeInfo.addProperty(propertyMetaInfo);
    }
}

} // namespace Internal

/*!
  \class SubComponentManager

  Detects & monitors (potential) component files in a list of directories, and registers
  these in the metatype system.
*/

SubComponentManager::SubComponentManager(MetaInfo metaInfo, QObject *parent) :
        QObject(parent),
        m_d(new Internal::SubComponentManagerPrivate(metaInfo, this))
{
}

SubComponentManager::~SubComponentManager()
{
    delete m_d;
}

QStringList SubComponentManager::directories() const
{
    return m_d->m_watcher.directories();
}

QStringList SubComponentManager::qmlFiles() const
{
    return m_d->m_watcher.files();
}

static bool importEqual(const QmlDomImport &import1, const QmlDomImport &import2)
{
    return import1.type() == import2.type()
           && import1.uri() == import2.uri()
           && import1.version() == import2.version()
           && import1.qualifier() == import2.qualifier();
}

void SubComponentManager::update(const QUrl &filePath, const QByteArray &data)
{
    QmlEngine engine;
    QmlDomDocument document;

    QList<QmlDomImport> imports;
    if (document.load(&engine, data, filePath))
        imports = document.imports();

    update(filePath, imports);
}

void SubComponentManager::update(const QUrl &filePath, const QList<QmlDomImport> &imports)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << filePath << imports.size();

    QFileInfo oldDir, newDir;

    if (!m_d->m_filePath.isEmpty()) {
        const QString file = m_d->m_filePath.toLocalFile();
        oldDir = QFileInfo(QFileInfo(file).path());
    }
    if (!filePath.isEmpty()) {
        const QString file = filePath.toLocalFile();
        newDir = QFileInfo(QFileInfo(file).path());
    }

    m_d->m_filePath = filePath;

    //
    // (implicit) import of local directory
    //
    if (oldDir != newDir) {
        if (!oldDir.filePath().isEmpty()) {
            m_d->m_dirToQualifier.remove(oldDir, QString());
            if (!m_d->m_dirToQualifier.contains(oldDir))
                m_d->m_watcher.removePath(oldDir.filePath());
        }

        if (!newDir.filePath().isEmpty()) {
            m_d->m_watcher.addPath(newDir.filePath());
            m_d->m_dirToQualifier.insertMulti(newDir, QString());
        }
    }

    //
    // Imports
    //

    // skip first list items until the lists differ
    int i = 0;
    while (i < qMin(imports.size(), m_d->m_imports.size())) {
        if (!importEqual(imports.at(i), m_d->m_imports.at(i)))
            break;
        ++i;
    }

    for (int ii = m_d->m_imports.size() - 1; ii >= i; --ii)
        m_d->removeImport(ii);

    for (int ii = i; ii < imports.size(); ++ii) {
        m_d->addImport(ii, imports.at(ii));
    }

    m_d->parseDirectories();
}

} // namespace QmlDesigner

#include "subcomponentmanager.moc"
