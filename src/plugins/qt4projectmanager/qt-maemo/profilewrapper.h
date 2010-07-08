#ifndef PROFILEWRAPPER_H
#define PROFILEWRAPPER_H

#include <QtCore/QDir>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class ProFile;
struct ProFileOption;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class ProFileReader;

class ProFileWrapper
{
public:
    ProFileWrapper(const QString &proFileName, const QString &qConfigFile);
    ~ProFileWrapper();

    void reload();

    struct InstallsElem {
        InstallsElem(QString v, QString p, QStringList f)
            : varName(v), path(p), files(f) {}
        QString varName;
        QString path;
        QStringList files;
    };

    struct InstallsList {
        QString targetPath;
        QList<InstallsElem> normalElems;
    };

    // High-level functions for dealing with INSTALLS stuff.
    InstallsList installs() const;
    bool addInstallsElem(const QString &path, const QString &file,
        const QString &var = QString()); // Empty var means make the name up.
    bool addInstallsTarget(const QString &path);
    bool removeInstallsElem(const QString &path, const QString &file);
    bool replaceInstallPath(const QString &oldPath, const QString &file,
        const QString &newPath);

    // Lower-level functions working on arbitrary variables.
    QStringList varValues(const QString &var) const;
    bool addVarValue(const QString &var, const QString &value);
    bool addFile(const QString &var, const QString &absFilePath);
    bool removeVarValue(const QString &var, const QString &value);
    bool removeFile(const QString &var, const QString &absFilePath);
    bool replaceVarValue(const QString &var, const QString &oldValue,
        const QString &newValue);

    QString absFilePath(const QString &relFilePath) const;

private:
    enum ParseType { ParseFromFile, ParseFromLines };
    void parseProFile(ParseType type) const;
    bool writeProFileContents();
    bool readProFileContents();
    InstallsElem findInstallsElem(const QString &path,
        const QString &file) const;

    const QString m_proFileName;
    const QDir m_proDir;
    mutable QStringList m_proFileContents;
    const QScopedPointer<ProFileOption> m_proFileOption;
    mutable QScopedPointer<ProFileReader> m_proFileReader;
    mutable ProFile *m_proFile;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEWRAPPER_H
