// main.qml - QML interface for the weather app
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtLocation 5.15
import QtPositioning 5.15

ApplicationWindow {
    id: window
    visible: true
    width: 400
    height: 600
    title: "Hyperlocal Weather Forecasting"

    // Weather API instance
    WeatherAPI {
        id: weatherAPI

        onForecastReady: {
            console.log("Forecast ready:", JSON.stringify(forecast))
        }

        onAlertTriggered: {
            alertDialog.alertText = message
            alertDialog.open()
        }
    }

    // Position source for GPS
    PositionSource {
        id: positionSource
        updateInterval: 30000  // Update every 30 seconds
        active: true

        onPositionChanged: {
            var coord = positionSource.position.coordinate
            weatherAPI.updateLocationData(coord.latitude, coord.longitude)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20

        Text {
            text: "Hyperlocal Weather"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        // Current conditions display
        GroupBox {
            title: "Current Conditions"
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                anchors.fill: parent

                Text { text: "Temperature:" }
                Text { 
                    text: weatherAPI.temperature
                    font.bold: true
                }

                Text { text: "Humidity:" }
                Text { 
                    text: weatherAPI.humidity
                    font.bold: true
                }

                Text { text: "Precipitation:" }
                Text { 
                    text: weatherAPI.precipitation
                    font.bold: true
                }

                Text { text: "Confidence:" }
                Text { 
                    text: (weatherAPI.confidence * 100).toFixed(1) + "%"
                    font.bold: true
                    color: weatherAPI.confidence > 0.85 ? "green" : "orange"
                }
            }
        }

        // Map view for hyperlocal visualization
        Map {
            id: map
            Layout.fillWidth: true
            Layout.fillHeight: true

            plugin: Plugin {
                name: "osm"  // OpenStreetMap
            }

            center: QtPositioning.coordinate(30.6280, -96.3344)  // College Station, TX
            zoomLevel: 12

            MapQuickItem {
                id: currentLocationMarker
                visible: positionSource.position.coordinate.isValid
                coordinate: positionSource.position.coordinate

                sourceItem: Rectangle {
                    width: 20
                    height: 20
                    radius: 10
                    color: "red"
                    border.color: "white"
                    border.width: 2
                }
            }
        }

        Button {
            text: "Update Forecast"
            Layout.fillWidth: true
            onClicked: {
                if (positionSource.position.coordinate.isValid) {
                    var coord = positionSource.position.coordinate
                    weatherAPI.generateHyperlocalForecast(coord.latitude, coord.longitude)
                }
            }
        }
    }

    // Alert dialog
    Dialog {
        id: alertDialog
        title: "Weather Alert"
        property string alertText: ""

        Text {
            text: alertDialog.alertText
            wrapMode: Text.WordWrap
        }

        standardButtons: Dialog.Ok
    }
}