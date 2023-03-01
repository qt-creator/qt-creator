@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef @SINGLE_INCLUDE_GUARD@
#define @SINGLE_INCLUDE_GUARD@
@endif

#include <QDesignerCustomWidgetInterface>

class @PLUGIN_CLASS@ : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
    @SINGLE_PLUGIN_METADATA@

public:
    explicit @PLUGIN_CLASS@(QObject *parent = nullptr);

    bool isContainer() const override;
    bool isInitialized() const override;
    QIcon icon() const override;
    QString domXml() const override;
    QString group() const override;
    QString includeFile() const override;
    QString name() const override;
    QString toolTip() const override;
    QString whatsThis() const override;
    QWidget *createWidget(QWidget *parent) override;
    void initialize(QDesignerFormEditorInterface *core) override;

private:
    bool m_initialized = false;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // @SINGLE_INCLUDE_GUARD@
@endif
