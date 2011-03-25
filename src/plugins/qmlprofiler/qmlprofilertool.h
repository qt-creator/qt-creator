#ifndef QMLPROFILERTOOL_H
#define QMLPROFILERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/ianalyzerengine.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool : public Analyzer::IAnalyzerTool
{
    Q_OBJECT
public:
    explicit QmlProfilerTool(QObject *parent = 0);
    ~QmlProfilerTool();

    QString id() const;
    QString displayName() const;
    ToolMode mode() const;

    void initialize(ExtensionSystem::IPlugin *plugin);

    Analyzer::IAnalyzerEngine *createEngine(ProjectExplorer::RunConfiguration *runConfiguration);

    Analyzer::IAnalyzerOutputPaneAdapter *outputPaneAdapter();
    QWidget *createToolBarWidget();
    QWidget *createTimeLineWidget();

public slots:
    void connectClient();
    void disconnectClient();

    void stopRecording();

    void gotoSourceLocation(const QString &fileName, int lineNumber);
    void updateTimer(qreal elapsedSeconds);

signals:
    void setTimeLabel(const QString &);

public:
    // Todo: configurable parameters
    static QString host;
    static quint16 port;

private:
    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTOOL_H
