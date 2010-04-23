#ifndef QMLJSPREVIEWRUNNER_H
#define QMLJSPREVIEWRUNNER_H

#include <QObject>

#include <projectexplorer/applicationlauncher.h>

namespace QmlJSEditor {
namespace Internal {

class QmlJSPreviewRunner : public QObject
{
    Q_OBJECT
public:
    explicit QmlJSPreviewRunner(QObject *parent = 0);
    void run(const QString &filename);

signals:

public slots:

private:
    QString m_qmlViewerDefaultPath;

    ProjectExplorer::ApplicationLauncher m_applicationLauncher;

};


} // namespace Internal
} // namespace QmlJSEditor


#endif // QMLJSPREVIEWRUNNER_H
