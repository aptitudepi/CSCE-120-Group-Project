import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import QtPositioning 6.0
import HyperlocalWeather 1.0

Rectangle {
    id: locationInput
    color: "white"
    radius: 10
    border.color: "#e0e0e0"
    border.width: 1
    implicitHeight: contentLayout.implicitHeight + 30

    signal locationSelected(double latitude, double longitude)

    PositionSource {
        id: positionSource
        active: false
        updateInterval: 10000
        preferredPositioningMethods: PositionSource.AllPositioningMethods
        
        onPositionChanged: {
            var coord = positionSource.position.coordinate;
            if (!coord || !coord.isValid || !isFinite(coord.latitude) || !isFinite(coord.longitude)) {
                console.warn("GPS returned invalid coordinate", coord);
                active = false;
                return;
            }
            var roundedLat = Number(coord.latitude.toFixed(4));
            var roundedLon = Number(coord.longitude.toFixed(4));
            console.log("GPS Position received:", roundedLat, roundedLon);
            locationSelected(roundedLat, roundedLon);
            latField.text = roundedLat.toFixed(4);
            lonField.text = roundedLon.toFixed(4);
            active = false; // Stop after getting position
        }
        
        onSourceErrorChanged: {
            if (sourceError != PositionSource.NoError) {
                console.log("GPS Error:", sourceError);
                active = false;
            }
        }
    }

    ColumnLayout {
        id: contentLayout
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
                text: positionSource.active ? qsTr("Locating...") : qsTr("Use GPS")
                enabled: !positionSource.active
                onClicked: {
                    console.log("Starting GPS location...")
                    positionSource.active = true
                    positionSource.update()
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
