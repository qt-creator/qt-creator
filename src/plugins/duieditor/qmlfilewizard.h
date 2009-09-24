#ifndef QMLFILEWIZARD_H
#define QMLFILEWIZARD_H

#include <coreplugin/basefilewizard.h>

namespace DuiEditor {

class QmlFileWizard: public Core::StandardFileWizard
{
    Q_OBJECT

public:
    typedef Core::BaseFileWizardParameters BaseFileWizardParameters;

    QmlFileWizard(const BaseFileWizardParameters &parameters,
                  QObject *parent = 0);

protected:
    QString fileContents(const QString &baseName) const;

protected:
    Core::GeneratedFiles generateFilesFromPath(const QString &path,
                                               const QString &fileName,
                                               QString *errorMessage) const;
};

} // namespace DuiEditor

#endif // QMLFILEWIZARD_H
