#ifndef MODIFIEDFILELISTER_H
#define MODIFIEDFILELISTER_H

#include <coreplugin/dialogs/ioptionspage.h>
class ModifiedFileLister : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ModifiedFileLister(QObject *parent = 0);
    ~ModifiedFileLister();
    // IOptionsPage implementation
    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
};
#endif // MODIFIEDFILELISTER_H
