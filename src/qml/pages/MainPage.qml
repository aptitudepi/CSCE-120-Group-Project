import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtLocation 6.4
import QtPositioning 6.4
import HyperlocalWeather 1.0

Page {
    id: mainPage

    function formatCountdown(seconds) {
        var total = Math.max(0, seconds || 0)
        var minutes = Math.floor(total / 60)
        var secs = total % 60
        var mm = minutes < 10 ? "0" + minutes : minutes
        var ss = secs < 10 ? "0" + secs : secs
        return mm + ":" + ss
    }

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
    property int mapOverlayTrim: 48

    property bool alertsInitialized: false
    property bool defaultAlertConfigured: false

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

    Flickable {
        id: mainScroll
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainColumn.implicitHeight + 40
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: mainColumn
            x: 20
            y: 20
            width: Math.max(mainScroll.width - 40, 0)
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

        // Alert monitoring status
        Rectangle {
            id: alertMonitorCard
            Layout.fillWidth: true
            Layout.preferredHeight: monitorLayout.implicitHeight + 24
            color: "#fff8e1"
            radius: 8
            border.color: "#ffe082"
            border.width: 1

            property int alertCount: alertController.alerts ? alertController.alerts.length : 0
            readonly property bool hasAlerts: alertCount > 0

            ColumnLayout {
                id: monitorLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: qsTr("Alert Monitor")
                        font.pixelSize: 18
                        font.bold: true
                        color: "#8a6d3b"
                    }

                    Rectangle {
                        radius: 10
                        color: alertMonitorCard.hasAlerts ? "#ffb300" : "#ffe082"
                        Layout.preferredHeight: 22
                        Layout.preferredWidth: 84

                        Text {
                            anchors.centerIn: parent
                            text: alertMonitorCard.hasAlerts ? qsTr("%1 active").arg(alertMonitorCard.alertCount) : qsTr("No alerts")
                            font.pixelSize: 11
                            font.bold: true
                            color: alertMonitorCard.hasAlerts ? "white" : "#8a6d3b"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("Check Now")
                        font.pixelSize: 12
                        implicitHeight: 30
                        enabled: alertController.monitoring
                        onClicked: {
                            alertController.checkAlerts()
                            alertController.startMonitoring()
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: formatCountdown(alertController.secondsToNextCheck)
                    font.pixelSize: 30
                    font.bold: true
                    color: "#ef6c00"
                    horizontalAlignment: Text.AlignLeft
                }

                ProgressBar {
                    Layout.fillWidth: true
                    implicitHeight: 6
                    from: 0
                    to: 1
                    value: alertController.checkIntervalSeconds > 0
                           ? 1 - Math.min(Math.max(alertController.secondsToNextCheck, 0),
                                   alertController.checkIntervalSeconds) / alertController.checkIntervalSeconds
                           : 0
                }

                Text {
                    Layout.fillWidth: true
                    text: alertController.monitoring
                          ? (alertMonitorCard.hasAlerts
                             ? qsTr("Next automatic check in %1 seconds").arg(Math.max(alertController.secondsToNextCheck, 0))
                             : qsTr("Add an alert to begin monitoring."))
                          : qsTr("Alert monitoring paused")
                    font.pixelSize: 11
                    color: "#8a6d3b"
                    wrapMode: Text.WordWrap
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
                if (!mainPage.alertsInitialized) {
                    alertController.startMonitoring()
                    mainPage.alertsInitialized = true
                }
                if (!mainPage.defaultAlertConfigured &&
                        (!alertController.alerts || alertController.alerts.length === 0)) {
                    var defaultThreshold = weatherController.current
                            ? Math.round(weatherController.current.temperature + 2)
                            : 95
                    alertController.addAlert(constrained.latitude, constrained.longitude,
                                             "temperature", defaultThreshold)
                    mainPage.defaultAlertConfigured = true
                }
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
                Layout.preferredHeight: 220
                Layout.minimumHeight: 200
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                clip: true

                Map {
                    id: locationMap
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    anchors.topMargin: -mainPage.mapOverlayTrim
                    center: mainPage.selectedCoordinate
                    zoomLevel: mainPage.hasSelectedCoordinate ? 12 : 11
                    copyrightsVisible: true
                    minimumZoomLevel: 10
                    maximumZoomLevel: 14

                    function selectApprovedMapType() {
                        if (!supportedMapTypes || supportedMapTypes.length === 0)
                            return

                        for (var i = 0; i < supportedMapTypes.length; ++i) {
                            var typeName = supportedMapTypes[i].name.toLowerCase()
                            if (typeName.indexOf("street") !== -1 ||
                                typeName.indexOf("custom") !== -1 ||
                                typeName.indexOf("osm") !== -1) {
                                if (activeMapType !== supportedMapTypes[i])
                                    activeMapType = supportedMapTypes[i]
                                return
                            }
                        }

                        if (activeMapType !== supportedMapTypes[0])
                            activeMapType = supportedMapTypes[0]
                    }

                    Component.onCompleted: selectApprovedMapType()
                    onSupportedMapTypesChanged: selectApprovedMapType()
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
                          ? qsTr("Outside coverage. Showing nearest point at %1°, %2°.")
                                .arg(mainPage.selectedCoordinate.latitude.toFixed(3))
                                .arg(mainPage.selectedCoordinate.longitude.toFixed(3))
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

    Popup {
        id: monitorPopup
        x: (parent.width - width) / 2
        y: 20
        width: Math.min(mainPage.width * 0.45, 320)
        implicitHeight: monitorPopupLayout.implicitHeight + 32
        modal: false
        focus: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: "white"
            radius: 10
            border.color: "#ffe082"
            border.width: 1
        }

        ColumnLayout {
            id: monitorPopupLayout
            anchors.fill: parent
            anchors.margins: 16
            spacing: 6

            Text {
                id: monitorPopupTitle
                text: qsTr("Alert Cycle")
                font.pixelSize: 16
                font.bold: true
                color: "#8a6d3b"
            }

            Text {
                id: monitorPopupMessage
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#5d4037"
                font.pixelSize: 13
            }
        }
    }

    Timer {
        id: monitorPopupAutoClose
        interval: 2500
        running: false
        repeat: false
        onTriggered: monitorPopup.close()
    }

    Popup {
        id: alertPopup
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: Math.min(mainPage.width * 0.6, 420)
        height: 230
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#ffcc80"
            border.width: 2
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            Text {
                id: alertPopupTitle
                text: qsTr("Weather Alert")
                font.pixelSize: 22
                font.bold: true
                color: "#e65100"
            }

            Text {
                id: alertPopupLocation
                Layout.fillWidth: true
                font.pixelSize: 14
                color: "#6d4c41"
                wrapMode: Text.WordWrap
            }

            Text {
                id: alertPopupMessage
                Layout.fillWidth: true
                font.pixelSize: 16
                wrapMode: Text.WordWrap
                color: "#4e342e"
            }

            Item { Layout.fillHeight: true }

            Button {
                text: qsTr("Dismiss")
                Layout.alignment: Qt.AlignRight
                onClicked: alertPopup.close()
            }
        }
    }

    Connections {
        target: alertController
        function onAlertTriggered(alert, message) {
            monitorPopup.close()
            monitorPopupAutoClose.stop()
            alertPopupTitle.text = alert && alert.alertType
                ? qsTr("%1 Alert").arg(alert.alertType)
                : qsTr("Weather Alert")
            alertPopupLocation.text = alert
                ? qsTr("Lat: %1°, Lon: %2°")
                    .arg(alert.latitude ? alert.latitude.toFixed(3) : "--")
                    .arg(alert.longitude ? alert.longitude.toFixed(3) : "--")
                : ""
            alertPopupMessage.text = message || qsTr("Alert conditions were met.")
            alertPopup.open()
        }
        function onAlertCycleStarted(activeAlerts) {
            monitorPopupTitle.text = qsTr("Alert Cycle")
            monitorPopupMessage.text = activeAlerts > 0
                ? qsTr("Checking %1 active alert%2 now.")
                    .arg(activeAlerts)
                    .arg(activeAlerts === 1 ? "" : "s")
                : qsTr("No active alerts to check right now.")
            monitorPopup.open()
            monitorPopupAutoClose.restart()
        }
    }
}

