/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef FOLDERNAVIGATIONWIDGET_H
#define FOLDERNAVIGATIONWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtGui/QWidget>
#include <QtGui/QListView>
#include <QtGui/QDirModel>
#include <QtGui/QLabel>
#include <QtGui/QSortFilterProxyModel>

namespace Core {
class ICore;
}

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;
class Node;

namespace Internal {

class FolderNavigationWidget : public QWidget {
    Q_OBJECT
public:
    FolderNavigationWidget(Core::ICore *core, QWidget *parent = 0);

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync);

    QString currentFolder() const;

public slots:
    void toggleAutoSynchronization();

private slots:
    void openItem(const QModelIndex &mainIndex);
    void setCurrentFile(const QString &filePath);

private:
    void setCurrentTitle(const QDir &directory);

    Core::ICore *m_core;
    ProjectExplorerPlugin *m_explorer;
    QListView *m_view;
    QDirModel *m_dirModel;
    QSortFilterProxyModel *m_filter;
    QLabel *m_title;
    bool m_autoSync;
};

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    FolderNavigationWidgetFactory(Core::ICore *core);
    virtual ~FolderNavigationWidgetFactory();

    virtual QString displayName();
    virtual QKeySequence activationSequence();
    virtual Core::NavigationView createWidget();
private:
    Core::ICore *m_core;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // FOLDERNAVIGATIONWIDGET_H
