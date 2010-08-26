#ifndef MODELCLASSWIZARD_H
#define MODELCLASSWIZARD_H

#include <coreplugin/basefilewizard.h>


class ModelClassWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit ModelClassWizard(const Core::BaseFileWizardParameters &parameters, QObject *parent = 0);
    ~ModelClassWizard();

    QWizard *createWizardDialog(QWidget *parent,
                                const QString &defaultPath,
                                const WizardPageList &extensionPages) const;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;

private:
    QString readFile(const QString& fileName,
                     const QMap<QString,QString>& replacementMap) const;
};

#endif // MODELCLASSWIZARD_H
