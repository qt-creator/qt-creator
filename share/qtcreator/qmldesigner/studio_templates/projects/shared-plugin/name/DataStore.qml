pragma Singleton
import QtQuick 6.5
import QtQuick.Studio.Utils 1.0

JsonListModel {
    id: models
    source: Qt.resolvedUrl("models.json")

    property ChildListModel exampleModel: ChildListModel {
        modelName: "exampleModel"
    }

    property JsonData backend: JsonData {}
}

