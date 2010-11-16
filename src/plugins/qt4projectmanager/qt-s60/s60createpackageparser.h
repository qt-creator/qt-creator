#ifndef SIGNSISPARSER_H
#define SIGNSISPARSER_H

#include <projectexplorer/ioutputparser.h>

#include <QtCore/QRegExp>

namespace Qt4ProjectManager {
namespace Internal {

class S60CreatePackageParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    S60CreatePackageParser(const QString &packageName);

    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    bool needPassphrase() const;

signals:
    void packageWasPatched(const QString &, const QStringList &pachingLines);

private:
    bool parseLine(const QString &line);

    const QString m_packageName;

    QRegExp m_signSis;
    QStringList m_patchingLines;

    bool m_needPassphrase;
};

} // namespace Internal
} // namespace Qt4ProjectExplorer


#endif // SIGNSISPARSER_H
