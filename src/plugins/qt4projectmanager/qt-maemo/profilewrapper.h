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
    ProFileWrapper(const QString &proFileName);
    ~ProFileWrapper();

    // TODO: Higher-level versions of these specifically for INSTALLS vars
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

    const QString m_proFileName;
    const QDir m_proDir;
    mutable QStringList m_proFileContents;
    QScopedPointer<ProFileOption> m_proFileOption;
    mutable QScopedPointer<ProFileReader> m_proFileReader;
    mutable ProFile *m_proFile;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEWRAPPER_H
