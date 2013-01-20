#ifndef VCPROJECTMANAGER_INTERNAL_MSBUILDPARSER_H
#define VCPROJECTMANAGER_INTERNAL_MSBUILDPARSER_H

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>
#include <QRegExp>

namespace VcProjectManager {
namespace Internal {

class MsBuildParser : public ProjectExplorer::IOutputParser
{
public:
    MsBuildParser();
    void stdOutput(const QString &line);

private:
    quint32 m_counter;
    QRegExp m_buildStartTimeRegExp;
    QRegExp m_compileWarningRegExp;
    QRegExp m_compileErrorRegExp;
    QRegExp m_doneTargetBuildRegExp;
    QRegExp m_doneProjectBuildRegExp;
    QRegExp m_buildSucceededRegExp;
    QRegExp m_buildFailedRegExp;
    QRegExp m_buildTimeElapsedRegExp;
    QRegExp m_msBuildErrorRegExp;

    bool m_buildAttempFinished;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_MSBUILDPARSER_H
