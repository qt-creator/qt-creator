#ifndef QMLPROFILERENGINE_H
#define QMLPROFILERENGINE_H

#include <analyzerbase/ianalyzerengine.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerEngine : public Analyzer::IAnalyzerEngine
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

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERENGINE_H
