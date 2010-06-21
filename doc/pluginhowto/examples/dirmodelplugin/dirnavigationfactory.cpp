#include "dirnavigationfactory.h"
#include "filesystemmodel.h"

#include <QTreeView>
#include <QDir>

#include <coreplugin/navigationwidget.h>
/**
 \todo
*/
Core::NavigationView DirNavigationFactory::createWidget()
{
    Core::NavigationView view;

    // Create FileSystemModel and set the defauls path as home path
    FileSystemModel* model = new FileSystemModel;
    model->setRootPath(QDir::homePath());

    // Create TreeView and set model
    QTreeView* tree = new QTreeView;
    tree->setModel(model);

    view.widget = tree;

    return view;
}

/**
 \todo
*/
QString DirNavigationFactory::displayName()
{
    return "Dir View";
}

