#ifndef QMLPROFILERENGINE_H
#define QMLPROFILERENGINE_H

#include "ianalyzerengine.h"

namespace Analyzer {
namespace Internal {


class QmlProfilerEngine : public IAnalyzerEngine
{
    Q_OBJECT
public:
    explicit QmlProfilerEngine(ProjectExplorer::RunConfiguration *runConfiguration);
    ~QmlProfilerEngine();

signals:
    void processRunning();
    void processTerminated();
    void stopRecording();

public slots:
    void start();
    void stop();
    void spontaneousStop();

    void viewUpdated();

private:
    class QmlProfilerEnginePrivate;
    QmlProfilerEnginePrivate *d;
};

}
}

#endif // QMLPROFILERENGINE_H
