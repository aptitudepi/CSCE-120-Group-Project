import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import HyperlocalWeather 1.0

Rectangle {
    id: forecastCard
    height: 120
    color: "white"
    radius: 8
    border.color: "#e0e0e0"
    border.width: 1

    property var weatherData

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Time/Date
        Column {
            Layout.preferredWidth: 150
            spacing: 5

            Text {
                text: weatherData ? 
                    Qt.formatDateTime(weatherData.timestamp, "MMM dd") : ""
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                text: weatherData ? 
                    Qt.formatDateTime(weatherData.timestamp, "hh:mm") : ""
                font.pixelSize: 14
                color: "#666"
            }
        }

        // Temperature
        Text {
            Layout.preferredWidth: 100
            text: weatherData ? 
                qsTr("%1°F").arg(Math.round(weatherData.temperature)) : ""
            font.pixelSize: 24
            font.bold: true
            color: "#2196f3"
        }

        // Condition
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            text: weatherData ? weatherData.weatherCondition : ""
            font.pixelSize: 16
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }

        // Details
        Row {
            Layout.preferredWidth: 200
            spacing: 15

            Text {
                text: weatherData ? 
                    qsTr("💧 %1%").arg(Math.round(weatherData.precipProbability * 100)) : ""
                font.pixelSize: 14
                color: "#666"
            }

            Text {
                text: weatherData ? 
                    qsTr("💨 %1 mph").arg(Math.round(weatherData.windSpeed)) : ""
                font.pixelSize: 14
                color: "#666"
            }
        }
    }
}

