#ifndef SCRIPTBINDINGREWRITER_H
#define SCRIPTBINDINGREWRITER_H

#include <QObject>
#include <QWeakPointer>

#include <private/qdeclarativedebug_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugObjectReference)

namespace Core {
    class IEditor;
}

namespace QmlJS {
    class ModelManagerInterface;
}

namespace QmlJSEditor {
namespace Internal {
    class QmlJSTextEditor;
}
}

namespace QmlJSInspector {
namespace Internal {

class QmlJSLiveTextPreview : public QObject
{
    Q_OBJECT

public:
    explicit QmlJSLiveTextPreview(QObject *parent = 0);
    static QmlJS::ModelManagerInterface *modelManager();
    void updateDocuments();

    void setActiveObject(const QDeclarativeDebugObjectReference &object);
    void mapObjectToQml(const QDeclarativeDebugObjectReference &object);

    QHash<QString, QHash<QmlJS::AST::UiObjectMember *, QList< QDeclarativeDebugObjectReference> > > m_initialTable;
    QHash< QmlJS::AST::UiObjectMember*, QList<QDeclarativeDebugObjectReference > > m_debugIds;

signals:
    void selectedItemsChanged(const QList<QDeclarativeDebugObjectReference> &objects);

private slots:
    void changeSelectedElements(QList<int> offsets, const QString &wordAtCursor);
    void documentChanged(QmlJS::Document::Ptr doc);
    void setEditor(Core::IEditor *editor);

private:
    QVariant castToLiteral(const QString &expression, QmlJS::AST::UiScriptBinding *scriptBinding);

private:
    QmlJS::Document::Ptr m_previousDoc;

    QmlJS::Snapshot m_snapshot;
    QWeakPointer<QmlJSEditor::Internal::QmlJSTextEditor> m_currentEditor;

};

} // namespace Internal
} // namespace QmlJSInspector

#endif // SCRIPTBINDINGREWRITER_H
