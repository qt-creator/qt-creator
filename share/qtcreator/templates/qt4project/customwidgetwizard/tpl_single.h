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
    @PLUGIN_CLASS@(QObject *parent = 0);

    bool isContainer() const;
    bool isInitialized() const;
    QIcon icon() const;
    QString domXml() const;
    QString group() const;
    QString includeFile() const;
    QString name() const;
    QString toolTip() const;
    QString whatsThis() const;
    QWidget *createWidget(QWidget *parent);
    void initialize(QDesignerFormEditorInterface *core);

private:
    bool m_initialized;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // @SINGLE_INCLUDE_GUARD@
@endif
