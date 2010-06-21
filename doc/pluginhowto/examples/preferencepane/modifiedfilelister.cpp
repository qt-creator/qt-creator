#include "modifiedfilelister.h"
#include "modifiedfilelistwidget.h"

ModifiedFileLister::ModifiedFileLister(QObject *parent): IOptionsPage(parent)
{

}

ModifiedFileLister::~ModifiedFileLister()
{

}

QString ModifiedFileLister::id() const
{
    return "ModifiedFiles";
}

QString ModifiedFileLister::trName() const
{
    return tr("Modified Files");
}

QString ModifiedFileLister::category() const
{
    return "Help";
}

QString ModifiedFileLister::trCategory() const
{
    return tr("Help");
}

QWidget *ModifiedFileLister::createPage(QWidget *parent)
{
    return new ModifiedFileListWidget(parent);
}

void ModifiedFileLister::apply()
{
    // Do nothing
}

void ModifiedFileLister::finish()
{
    // Do nothing
}
