#ifndef CLEARCASESYNC_H
#define CLEARCASESYNC_H

#include "clearcaseplugin.h"

namespace ClearCase {
namespace Internal {

class ClearCaseSync : public QObject
{
    Q_OBJECT
public:
    explicit ClearCaseSync(ClearCasePlugin *plugin, QSharedPointer<StatusMap> statusMap);
    void run(QFutureInterface<void> &future, const QString &topLevel, QStringList &files);

signals:
    void updateStreamAndView();
    void setStatus(const QString &file, ClearCase::Internal::FileStatus::Status status, bool update);

private:
    ClearCasePlugin *m_plugin;
    QSharedPointer<StatusMap> m_statusMap;
};

} // namespace Internal
} // namespace ClearCase

#endif // CLEARCASESYNC_H
