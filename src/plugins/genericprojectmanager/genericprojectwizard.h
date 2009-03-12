#ifndef GENERICPROJECTWIZARD_H
#define GENERICPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>

namespace GenericProjectManager {
namespace Internal {

class GenericProjectWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    GenericProjectWizard();
    virtual ~GenericProjectWizard();

    static Core::BaseFileWizardParameters parameters();

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage);
};

} // end of namespace Internal
} // end of namespace GenericProjectManager

#endif // GENERICPROJECTWIZARD_H
