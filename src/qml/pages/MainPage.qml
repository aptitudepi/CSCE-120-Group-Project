import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtLocation 6.4
import QtPositioning 6.4
import HyperlocalWeather 1.0

Page {
    id: mainPage

    readonly property var defaultCoordinate: QtPositioning.coordinate(30.6272, -96.3344)
    readonly property var allowedRegion: ({
        north: 31.3,
        south: 29.8,
        east: -95.5,
        west: -97.5
    })
    property var selectedCoordinate: defaultCoordinate
    property bool hasSelectedCoordinate: false
    property bool outsideAllowedRegion: false

    function clampToAllowedRegion(lat, lon) {
        var boundedLat = Math.min(Math.max(lat, allowedRegion.south), allowedRegion.north)
        var boundedLon = Math.min(Math.max(lon, allowedRegion.west), allowedRegion.east)
        return QtPositioning.coordinate(boundedLat, boundedLon)
    }

    function isOutOfBounds(lat, lon, boundedCoord) {
        return Math.abs(boundedCoord.latitude - lat) > 0.0001 ||
               Math.abs(boundedCoord.longitude - lon) > 0.0001
    }

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

                Button {
                    text: qsTr("Refresh")
                    onClicked: weatherController.refreshForecast()
                    enabled: !weatherController.loading
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: infoLayout.implicitHeight + 24
            Layout.bottomMargin: 10
            color: "#e3f2fd"
            radius: 6
            border.color: "#bbdefb"
            border.width: 1

            ColumnLayout {
                id: infoLayout
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 10

                Text {
                    text: qsTr("Interpolated Forecast")
                    font.pixelSize: 18
                    font.bold: true
                    color: "#0d47a1"
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Combines NWS and Pirate Weather data using a 1 km spatial grid (center + 6 offsets) "
                               + "and one-minute temporal interpolation between now and each provider's next forecast.")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                    color: "#0d47a1"
                }
            }
        }

        // Location Input
        LocationInput {
            id: locationInput
            Layout.fillWidth: true
            Layout.topMargin: 10
            onLocationSelected: function(lat, lon) {
                var constrained = mainPage.clampToAllowedRegion(lat, lon)
                mainPage.selectedCoordinate = constrained
                mainPage.hasSelectedCoordinate = true
                mainPage.outsideAllowedRegion = mainPage.isOutOfBounds(lat, lon, constrained)
                weatherController.fetchForecast(constrained.latitude, constrained.longitude)
            }
        }

        GridLayout {
            id: insightGrid
            Layout.fillWidth: true
            columns: mainPage.width > 980 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 20

            // Map Preview
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                Layout.minimumHeight: 240
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                clip: true

                Map {
                    id: locationMap
                    anchors.fill: parent
                    anchors.margins: 12
                    center: mainPage.selectedCoordinate
                    zoomLevel: mainPage.hasSelectedCoordinate ? 12 : 11
                    copyrightsVisible: true
                    gesture.enabled: false
                    plugin: Plugin {
                        name: "osm"
                        PluginParameter {
                            name: "osm.mapping.providersrepository.disabled"
                            value: true
                        }
                        PluginParameter {
                            name: "osm.mapping.custom.host"
                            value: "https://tile.openstreetmap.org/"
                        }
                        PluginParameter {
                            name: "osm.mapping.custom.copyright"
                            value: "© OpenStreetMap contributors"
                        }
                    }

                    MapRectangle {
                        color: "#2196f320"
                        border.color: "#2196f3"
                        topLeft: QtPositioning.coordinate(mainPage.allowedRegion.north, mainPage.allowedRegion.west)
                        bottomRight: QtPositioning.coordinate(mainPage.allowedRegion.south, mainPage.allowedRegion.east)
                    }

                    MapQuickItem {
                        coordinate: mainPage.selectedCoordinate
                        visible: mainPage.hasSelectedCoordinate
                        anchorPoint.x: sourceItem ? sourceItem.width / 2 : 0
                        anchorPoint.y: sourceItem ? sourceItem.height : 0
                        sourceItem: Rectangle {
                            width: 18
                            height: 18
                            radius: 9
                            color: "#d32f2f"
                            border.color: "white"
                            border.width: 2
                        }
                    }
                }

                Text {
                    text: mainPage.outsideAllowedRegion
                          ? qsTr("Location outside coverage. Showing nearest supported point.")
                          : qsTr("Select a location within the supported coverage area.")
                    color: mainPage.outsideAllowedRegion ? "#d32f2f" : "#666"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.centerIn: parent
                    padding: 16
                    visible: !mainPage.hasSelectedCoordinate || mainPage.outsideAllowedRegion
                    z: 1
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
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

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

