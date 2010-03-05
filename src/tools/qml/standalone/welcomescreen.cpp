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

#include "welcomescreen.h"
#include "application.h"

#include <QtGui/QBoxLayout>
#include <QtDeclarative/QmlView>
#include <QtDeclarative/QmlGraphicsItem>
#include <QtDeclarative/QmlContext>
#include <QtCore/QCoreApplication>

RecentFileModel::RecentFileModel(QObject *parent) :
        QListModelInterface(parent)
{
}

void RecentFileModel::setRecentFiles(const QStringList &filePaths)
{
    QStringList oldPaths = m_paths;
    m_paths.clear();
    if (oldPaths.size() > 0) {
        emit itemsRemoved(0, oldPaths.size());
    }

    m_paths = filePaths;

    if (m_paths.size() > 0) {
        emit itemsInserted(0, m_paths.size());
    }
}

int RecentFileModel::count() const
{
    return m_paths.size();
}

QHash<int, QVariant> RecentFileModel::data(int index, const QList<int> &/*roles*/) const
{
    QHash<int,QVariant> dataHash;

    dataHash.insert(NameRole, displayPath(m_paths.at(index)));
    dataHash.insert(FileNameRole, m_paths.at(index));
    return dataHash;
}

QVariant RecentFileModel::data(int index, int role) const
{
    if (role == NameRole)
        return displayPath(m_paths.at(index));
    if (role == FileNameRole)
        return m_paths.at(index);
    return QVariant();
}

QList<int> RecentFileModel::roles() const
{
    return QList<int>() << NameRole << FileNameRole;
}

QString RecentFileModel::toString(int role) const
{
    switch (role) {
    case NameRole: return "name"; break;
    case FileNameRole: return "fileName"; break;
    default: Q_ASSERT_X(0, Q_FUNC_INFO, "Unknown role");
    }
    return QString();
}

QString RecentFileModel::displayPath(const QString &filePath) const
{
    QString displayPath = filePath;
    while (displayPath.length() > 25 && displayPath.contains('/')) {
        displayPath.remove(0, displayPath.indexOf('/') + 1);
    }

    if (displayPath != filePath)
        displayPath.prepend("../");

    return displayPath;
}

WelcomeScreen::WelcomeScreen(QWidget *parent) :
        QWidget(parent),
        m_view(new QmlView(this)),
        m_recentFileModel(new RecentFileModel(this))
{
    m_view->setUrl(QUrl::fromLocalFile(Application::sharedDirPath() + "welcomescreen.qml"));
    m_view->setContentResizable(true);

    QmlContext *context = m_view->rootContext();
    context->setContextProperty(QLatin1String("recentFiles"), m_recentFileModel);

    m_view->execute();

    QObject *rootItem = m_view->root();
    connect(rootItem, SIGNAL(openFile()), this, SLOT(openFile()));

    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->setMargin(0);
    layout->addWidget(m_view);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x2e, 0x2e, 0x2e));
    setPalette(palette);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
}

void WelcomeScreen::setRecentFiles(const QStringList &files)
{
    m_recentFileModel->setRecentFiles(files);
}

void WelcomeScreen::openFile()
{
    QString filePath = m_view->root()->property("selectedFile").toString();
    if (filePath.startsWith(':')) {
        emit newFile(filePath);
    } else {
        emit openFile(filePath);
    }
}
