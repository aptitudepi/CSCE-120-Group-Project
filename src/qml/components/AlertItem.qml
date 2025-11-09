import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import HyperlocalWeather 1.0

Rectangle {
    id: alertItem
    height: 100
    color: "white"
    radius: 8
    border.color: "#e0e0e0"
    border.width: 1

    property var alertModel

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Column {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: alertModel ? alertModel.alertType : ""
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                text: alertModel ? 
                    qsTr("Location: %1, %2").arg(alertModel.latitude.toFixed(4))
                    .arg(alertModel.longitude.toFixed(4)) : ""
                font.pixelSize: 12
                color: "#666"
            }

            Text {
                text: alertModel ? 
                    qsTr("Threshold: %1").arg(alertModel.threshold) : ""
                font.pixelSize: 12
                color: "#666"
            }
        }

        Switch {
            checked: alertModel ? alertModel.enabled : false
            onToggled: {
                if (alertModel) {
                    alertController.toggleAlert(alertModel.id, checked)
                }
            }
        }

        Button {
            text: qsTr("Delete")
            onClicked: {
                if (alertModel) {
                    alertController.removeAlert(alertModel.id)
                }
            }
        }
    }
}

