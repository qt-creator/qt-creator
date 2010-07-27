#ifndef SIGNSISPARSER_H
#define SIGNSISPARSER_H

#include <projectexplorer/ioutputparser.h>

#include <QtCore/QRegExp>

namespace Qt4ProjectManager {

class SignsisParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    SignsisParser();

    virtual void stdOutput(const QString & line);
    virtual void stdError(const QString & line);

private:
    QRegExp m_signSis;
};

} // namespace Qt4ProjectExplorer


#endif // SIGNSISPARSER_H
