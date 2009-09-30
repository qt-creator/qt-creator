#include "qmleditorconstants.h"
#include "qmlfilewizard.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

using namespace QmlEditor;

QmlFileWizard::QmlFileWizard(const BaseFileWizardParameters &parameters,
                             QObject *parent):
    Core::StandardFileWizard(parameters, parent)
{
}

Core::GeneratedFiles QmlFileWizard::generateFilesFromPath(const QString &path,
                                                          const QString &name,
                                                          QString * /*errorMessage*/) const

{
    const QString mimeType = QLatin1String(Constants::QMLEDITOR_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(mimeType));

    Core::GeneratedFile file(fileName);
    file.setEditorKind(QLatin1String(Constants::C_QMLEDITOR));
    file.setContents(fileContents(fileName));

    return Core::GeneratedFiles() << file;
}

QString QmlFileWizard::fileContents(const QString &fileName) const
{
    const QString baseName = QFileInfo(fileName).completeBaseName();
    QString contents;
    QTextStream str(&contents);
//    str << CppTools::AbstractEditorSupport::licenseTemplate();

    str << QLatin1String("import Qt 4.6\n")
        << QLatin1String("\n")
        << QLatin1String("Rectangle {\n")
        << QLatin1String("    width: 640\n")
        << QLatin1String("    height: 480\n")
        << QLatin1String("}\n");

    return contents;
}
