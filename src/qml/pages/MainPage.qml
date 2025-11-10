import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import HyperlocalWeather 1.0

Page {
    id: mainPage
    
    signal settingsRequested()

    background: Rectangle {
        color: "#f0f0f0"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#e3f2fd" }
            GradientStop { position: 1.0; color: "#bbdefb" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#2196f3"
            radius: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20

                Text {
                    text: qsTr("Hyperlocal Weather")
                    font.pixelSize: 32
                    font.bold: true
                    color: "white"
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    id: serviceSelector
                    Layout.preferredWidth: 180
                    model: ["NWS", "PirateWeather", "Aggregated"]
                    currentIndex: {
                        var provider = weatherController.serviceProvider
                        if (provider === "NWS") return 0
                        if (provider === "PirateWeather") return 1
                        if (provider === "Aggregated") return 2
                        return 0
                    }
                    enabled: !weatherController.loading
                    
                    onCurrentIndexChanged: {
                        weatherController.setServiceProvider(currentIndex)
                    }
                    
                    Connections {
                        target: weatherController
                        function onServiceProviderChanged() {
                            var provider = weatherController.serviceProvider
                            if (provider === "NWS") serviceSelector.currentIndex = 0
                            else if (provider === "PirateWeather") serviceSelector.currentIndex = 1
                            else if (provider === "Aggregated") serviceSelector.currentIndex = 2
                        }
                    }
                }

                Button {
                    text: qsTr("Refresh")
                    onClicked: weatherController.refreshForecast()
                    enabled: !weatherController.loading
                }
                
                Button {
                    text: qsTr("Settings")
                    onClicked: mainPage.settingsRequested()
                }
            }
        }

        // Service Info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "white"
            radius: 5
            border.color: "#e0e0e0"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Text {
                    text: qsTr("Service: %1").arg(weatherController.serviceProvider)
                    font.pixelSize: 14
                    color: "#666"
                }
                
                Item { Layout.fillWidth: true }
            }
        }

        // Location Input
        LocationInput {
            id: locationInput
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            onLocationSelected: function(lat, lon) {
                weatherController.fetchForecast(lat, lon)
            }
        }

        // Current Weather Card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            color: "white"
            radius: 10
            border.color: "#e0e0e0"
            border.width: 1

            visible: weatherController.current !== null

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: weatherController.current ? 
                            qsTr("Current Weather") : ""
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Text {
                        text: weatherController.current ? 
                            qsTr("%1°F").arg(Math.round(weatherController.current.temperature)) : ""
                        font.pixelSize: 48
                        font.bold: true
                        color: "#2196f3"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: weatherController.current ? 
                            weatherController.current.weatherCondition : ""
                        font.pixelSize: 18
                        color: "#666"
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                        maximumLineCount: 2
                    }
                }

                ColumnLayout {
                    spacing: 10

                    Row {
                        spacing: 20
                        Text {
                            text: qsTr("Humidity: %1%").arg(
                                weatherController.current ? weatherController.current.humidity : 0)
                            font.pixelSize: 14
                        }
                        Text {
                            text: qsTr("Wind: %1 mph").arg(
                                weatherController.current ? 
                                Math.round(weatherController.current.windSpeed) : 0)
                            font.pixelSize: 14
                        }
                    }

                    Row {
                        spacing: 20
                        Text {
                            text: qsTr("Precip: %1%").arg(
                                weatherController.current ? 
                                Math.round(weatherController.current.precipProbability * 100) : 0)
                            font.pixelSize: 14
                        }
                        Text {
                            text: qsTr("Pressure: %1 hPa").arg(
                                weatherController.current ? 
                                Math.round(weatherController.current.pressure) : 0)
                            font.pixelSize: 14
                        }
                    }
                }
            }
        }

        // Loading indicator
        BusyIndicator {
            Layout.alignment: Qt.AlignCenter
            visible: weatherController.loading
            running: weatherController.loading
        }

        // Forecast List
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: forecastList
                model: weatherController.forecastModel
                spacing: 10
                clip: true

                delegate: ForecastCard {
                    width: forecastList.width
                    weatherData: model
                }
            }
        }
    }
}

