#ifndef CUSTOMPROJECTWIZARD_H
#define CUSTOMPROJECTWIZARD_H

#include <coreplugin/dialogs/iwizard.h>

class CustomProjectWizard : public Core::IWizard
{
public:
    CustomProjectWizard();
    ~CustomProjectWizard();

    Core::IWizard::Kind kind() const;
    QIcon icon() const;
    QString description() const;
    QString name() const;
    QString category() const;
    QString trCategory() const;
    QStringList runWizard(const QString &path, QWidget *parent);
};

#endif // CUSTOMPROJECTWIZARD_H
