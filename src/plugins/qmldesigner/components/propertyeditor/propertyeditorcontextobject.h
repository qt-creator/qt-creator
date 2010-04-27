#ifndef PROPERTYEDITORCONTEXTOBJECT_H
#define PROPERTYEDITORCONTEXTOBJECT_H

#include <QObject>
#include <QUrl>
#include <QDeclarativePropertyMap>

namespace QmlDesigner {

class PropertyEditorContextObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl globalBaseUrl READ globalBaseUrl WRITE setGlobalBaseUrl NOTIFY globalBaseUrlChanged)
    Q_PROPERTY(QUrl specificsUrl READ specificsUrl WRITE setSpecificsUrl NOTIFY specificsUrlChanged)

    Q_PROPERTY(QString specificQmlData READ specificQmlData WRITE setSpecificQmlData NOTIFY specificQmlDataChanged)
    Q_PROPERTY(QString stateName READ stateName WRITE setStateName NOTIFY stateNameChanged)

    Q_PROPERTY(bool isBaseState READ isBaseState WRITE setIsBaseState NOTIFY isBaseStateChanged)
    Q_PROPERTY(bool selectionChanged READ selectionChanged WRITE setSelectionChanged NOTIFY selectionChangedChanged)

    Q_PROPERTY(QDeclarativePropertyMap* backendValues READ backendValues WRITE setBackendValues NOTIFY backendValuesChanged)

public:
    PropertyEditorContextObject(QObject *parent = 0);

    QUrl globalBaseUrl() const {return m_globalBaseUrl; }
    QUrl specificsUrl() const {return m_specificsUrl; }
    QString specificQmlData() const {return m_specificQmlData; }
    QString stateName() const {return m_stateName; }

    bool isBaseState() const { return m_isBaseState; }
    bool selectionChanged() const { return m_selectionChanged; }

    QDeclarativePropertyMap* backendValues() const { return m_backendValues; }

signals:
    void globalBaseUrlChanged();
    void specificsUrlChanged();
    void specificQmlDataChanged();
    void stateNameChanged();
    void isBaseStateChanged();
    void selectionChangedChanged();
    void backendValuesChanged();

public slots:
     void setGlobalBaseUrl(const QUrl &newBaseUrl)
     {
         if (newBaseUrl == m_globalBaseUrl)
             return;

         m_globalBaseUrl = newBaseUrl;
         emit globalBaseUrlChanged();
     }

     void setSpecificsUrl(const QUrl &newSpecificsUrl)
     {
         if (newSpecificsUrl == m_specificsUrl)
             return;

         m_specificsUrl = newSpecificsUrl;
         emit specificsUrlChanged();
     }

     void setSpecificQmlData(const QString &newSpecificQmlData)
     {
         if (m_specificQmlData == newSpecificQmlData)
             return;

         m_specificQmlData = newSpecificQmlData;
         emit specificQmlDataChanged();
     }

     void setStateName(const QString &newStateName)
     {
         if (newStateName == m_stateName)
             return;

         m_stateName = newStateName;
         emit stateNameChanged();
     }

     void setIsBaseState(bool newIsBaseState)
     {
         if (newIsBaseState ==  m_isBaseState)
             return;

         m_isBaseState = newIsBaseState;
         emit isBaseStateChanged();
     }

     void setSelectionChanged(bool newSelectionChanged)
     {
         if (newSelectionChanged ==  m_selectionChanged)
             return;

         m_selectionChanged = newSelectionChanged;
         emit selectionChangedChanged();
     }

     void setBackendValues(QDeclarativePropertyMap* newBackendValues)
     {
         if (newBackendValues ==  m_backendValues)
             return;

         m_backendValues = newBackendValues;
         emit backendValuesChanged();
     }

    void triggerSelectionChanged()
    {
        setSelectionChanged(false);
        setSelectionChanged(true);
        setSelectionChanged(false);
    }

private:
    QUrl m_globalBaseUrl;
    QUrl m_specificsUrl;

    QString m_specificQmlData;
    QString m_stateName;

    bool m_isBaseState;
    bool m_selectionChanged;

    QDeclarativePropertyMap* m_backendValues;

};

} //QmlDesigner {

#endif // PROPERTYEDITORCONTEXTOBJECT_H
