#include "modifiedfilelistwidget.h"

#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>

ModifiedFileListWidget::ModifiedFileListWidget(QWidget* parent):QListWidget(parent)
{
    // Show the list of modified pages
    Core::FileManager* fm = Core::ICore::instance()->fileManager();
    QList<Core::IFile*> files = fm->modifiedFiles();

    for(int i=0; i<files.count();i++)
        this->addItem(files.at(i)->fileName());
}

ModifiedFileListWidget::~ModifiedFileListWidget()
{
    //Do Nothing
}
