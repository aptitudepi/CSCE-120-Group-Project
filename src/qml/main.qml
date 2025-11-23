import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material 2.15
import HyperlocalWeather 1.0

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: qsTr("Hyperlocal Weather")

    // Navigation stack
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainPage
        
        Component {
            id: mainPage
            MainPage { }
        }
    }

    // Error dialog (using popup instead of MessageDialog for Qt 6)
    Popup {
        id: errorDialog
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 400
        height: 200
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            Text {
                text: qsTr("Error")
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                id: errorText
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Button {
                text: qsTr("OK")
                onClicked: errorDialog.close()
            }
        }
    }

    Connections {
        target: weatherController
        function onErrorMessageChanged() {
            if (weatherController.errorMessage) {
                errorText.text = weatherController.errorMessage
                errorDialog.open()
            }
        }
    }
}

