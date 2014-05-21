/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "fileresourcesmodel.h"

#include <coreplugin/icore.h>

#include <model.h>

#include <QFileDialog>
#include <QDirIterator>
#include <qmlanchorbindingproxy.h>

static QString s_lastBrowserPath;

FileResourcesModel::FileResourcesModel(QObject *parent) :
    QObject(parent), m_filter("(*.*)"), m_lock(false)
{
}

void FileResourcesModel::setAnchorBackend(const QVariant &anchorBackend)
{

    QObject* anchorBackendObject = anchorBackend.value<QObject*>();

    const QmlDesigner::Internal::QmlAnchorBindingProxy *backendCasted =
            qobject_cast<const QmlDesigner::Internal::QmlAnchorBindingProxy *>(anchorBackendObject);

    if (backendCasted)
        m_path = backendCasted->getItemNode().modelNode().model()->fileUrl();

    setupModel();
    emit anchorBackendChanged();
}

QString FileResourcesModel::fileName() const
{
    return m_fileName.toString();
}

void FileResourcesModel::setFileNameStr(const QString &fileName)
{
    setFileName(QUrl(fileName));
}
void FileResourcesModel::setFileName(const QUrl &fileName)
{
    if (fileName == m_fileName)
        return;

    m_fileName = fileName;

    emit fileNameChanged(fileName);
}

void FileResourcesModel::setPath(const QUrl &url)
{
    m_path = url;
    setupModel();
}

QUrl FileResourcesModel::path() const
{
    return m_path;
}

void FileResourcesModel::setFilter(const QString &filter)
{
    m_filter = filter;
}

QString FileResourcesModel::filter() const
{
    return m_filter;
}

QStringList FileResourcesModel::fileModel() const
{
    if (m_model.isEmpty())
        return QStringList(QString());

    return m_model;
}

void FileResourcesModel::openFileDialog()
{
    QString modelPath;

    modelPath = m_path.toLocalFile();

    m_lastModelPath = modelPath;

    bool documentChanged = m_lastModelPath == modelPath;

    //First we try the last path this browser widget was opened with
    //if the document was not changed
    QString path = documentChanged ? QString() : m_currentPath;


    //If that one is not valid we try the path for the current file
    if (path.isEmpty() && !m_fileName.isEmpty())
        path = QFileInfo(modelPath + QStringLiteral("/") + m_fileName.toString()).absoluteDir().absolutePath();


    //Next we try to fall back to the path any file browser was opened with
    if (!QFileInfo(path).exists())
        path = s_lastBrowserPath;

    //The last fallback is to try the path of the document
    if (!QFileInfo(path).exists())
        path = modelPath;

    QString newFile = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Open File"), path, m_filter);

    if (!newFile.isEmpty()) {
        setFileNameStr(newFile);

        m_currentPath = QFileInfo(newFile).absolutePath();
        s_lastBrowserPath = m_currentPath;
    }
}

void FileResourcesModel::registerDeclarativeType()
{
    qmlRegisterType<FileResourcesModel>("HelperWidgets",2,0,"FileResourcesModel");
}

void FileResourcesModel::setupModel()
{
    m_lock = true;
    m_model.clear();

    QDir dir;

    dir = QFileInfo(m_path.toLocalFile()).dir();

    QStringList filterList = m_filter.split(' ');

    QDirIterator it(dir.absolutePath(), filterList, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString absolutePath = it.next();
        m_model.append(dir.relativeFilePath(absolutePath));
    }

    m_lock = false;
}
