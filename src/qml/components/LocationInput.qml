import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import HyperlocalWeather 1.0

Rectangle {
    id: locationInput
    color: "white"
    radius: 10
    border.color: "#e0e0e0"
    border.width: 1

    signal locationSelected(double latitude, double longitude)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        Text {
            text: qsTr("Location")
            font.pixelSize: 18
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: latField
                Layout.fillWidth: true
                placeholderText: qsTr("Latitude")
                validator: DoubleValidator {
                    bottom: -90
                    top: 90
                }
            }

            TextField {
                id: lonField
                Layout.fillWidth: true
                placeholderText: qsTr("Longitude")
                validator: DoubleValidator {
                    bottom: -180
                    top: 180
                }
            }

            Button {
                text: qsTr("Get Forecast")
                onClicked: {
                    var lat = parseFloat(latField.text)
                    var lon = parseFloat(lonField.text)
                    if (!isNaN(lat) && !isNaN(lon)) {
                        locationSelected(lat, lon)
                    }
                }
            }

            Button {
                text: qsTr("Use GPS")
                onClicked: {
                    // TODO: Implement GPS location
                    // For now, use a default location (College Station, TX)
                    locationSelected(30.6272, -96.3344)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Example: 30.6272, -96.3344 (College Station, TX)")
            font.pixelSize: 12
            color: "#666"
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
        }
    }
}

