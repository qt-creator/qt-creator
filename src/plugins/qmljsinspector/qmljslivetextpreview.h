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
    explicit QmlJSLiveTextPreview(QmlJS::Document::Ptr doc, QObject *parent = 0);
    static QmlJS::ModelManagerInterface *modelManager();
    //void updateDocuments();

    void associateEditor(Core::IEditor *editor);
    void unassociateEditor(Core::IEditor *editor);
    void setActiveObject(const QDeclarativeDebugObjectReference &object);
    void mapObjectToQml(const QDeclarativeDebugObjectReference &object);

signals:
    void selectedItemsChanged(const QList<QDeclarativeDebugObjectReference> &objects);

private slots:
    void changeSelectedElements(QList<int> offsets, const QString &wordAtCursor);
    void documentChanged(QmlJS::Document::Ptr doc);
    void updateDebugIds(const QDeclarativeDebugObjectReference &rootReference);

private:
    QList<QDeclarativeDebugObjectReference > objectReferencesForOffset(quint32 offset) const;
    QVariant castToLiteral(const QString &expression, QmlJS::AST::UiScriptBinding *scriptBinding);

private:
    QHash<QmlJS::AST::UiObjectMember*, QList<QDeclarativeDebugObjectReference> > m_initialTable;
    QHash<QmlJS::AST::UiObjectMember*, QList<QDeclarativeDebugObjectReference> > m_debugIds;

    QmlJS::Document::Ptr m_previousDoc;
    QString m_filename;

    QList<QWeakPointer<QmlJSEditor::Internal::QmlJSTextEditor> > m_editors;

};

} // namespace Internal
} // namespace QmlJSInspector

#endif // SCRIPTBINDINGREWRITER_H
