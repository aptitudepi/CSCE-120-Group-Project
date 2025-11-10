import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import HyperlocalWeather 1.0

Page {
    id: settingsPage
    
    background: Rectangle {
        color: "#f0f0f0"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#e3f2fd" }
            GradientStop { position: 1.0; color: "#bbdefb" }
        }
    }
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        
        ColumnLayout {
            width: settingsPage.width - 40
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
                        text: qsTr("Settings")
                        font.pixelSize: 32
                        font.bold: true
                        color: "white"
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        text: qsTr("Back")
                        onClicked: {
                            if (settingsPage.parentStackView) {
                                settingsPage.parentStackView.pop()
                            }
                        }
                    }
                }
            }
            
            // Service Provider Settings
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Text {
                        text: qsTr("Weather Service")
                        font.pixelSize: 20
                        font.bold: true
                    }
                    
                    ComboBox {
                        id: serviceCombo
                        Layout.fillWidth: true
                        model: ["NWS", "PirateWeather", "Aggregated"]
                        currentIndex: {
                            var provider = weatherController.serviceProvider
                            if (provider === "NWS") return 0
                            if (provider === "PirateWeather") return 1
                            if (provider === "Aggregated") return 2
                            return 0
                        }
                        
                        onCurrentIndexChanged: {
                            weatherController.setServiceProvider(currentIndex)
                        }
                    }
                    
                    CheckBox {
                        id: useAggregationCheck
                        text: qsTr("Use Multi-Source Aggregation")
                        checked: weatherController.useAggregation
                        
                        onCheckedChanged: {
                            weatherController.setUseAggregation(checked)
                        }
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Aggregation combines data from multiple weather services for improved accuracy")
                        font.pixelSize: 12
                        color: "#666"
                        wrapMode: Text.WordWrap
                    }
                }
            }
            
            // Performance Metrics
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 250
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                visible: weatherController.performanceMonitor !== null
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Text {
                        text: qsTr("Performance Metrics")
                        font.pixelSize: 20
                        font.bold: true
                    }
                    
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 20
                        
                        Text {
                            text: qsTr("Avg Response Time:")
                            font.pixelSize: 14
                        }
                        Text {
                            text: weatherController.performanceMonitor ? 
                                qsTr("%1s").arg(weatherController.performanceMonitor.forecastResponseTime.toFixed(2)) : "0.00s"
                            font.pixelSize: 14
                            font.bold: true
                        }
                        
                        Text {
                            text: qsTr("Precipitation Hit Rate:")
                            font.pixelSize: 14
                        }
                        Text {
                            text: weatherController.performanceMonitor ? 
                                qsTr("%1%").arg(Math.round(weatherController.performanceMonitor.precipitationHitRate * 100)) : "0%"
                            font.pixelSize: 14
                            font.bold: true
                        }
                        
                        Text {
                            text: qsTr("Service Uptime:")
                            font.pixelSize: 14
                        }
                        Text {
                            text: weatherController.performanceMonitor ? 
                                qsTr("%1%").arg(Math.round(weatherController.performanceMonitor.serviceUptime * 100)) : "100%"
                            font.pixelSize: 14
                            font.bold: true
                        }
                        
                        Text {
                            text: qsTr("Alert Lead Time:")
                            font.pixelSize: 14
                        }
                        Text {
                            text: weatherController.performanceMonitor ? 
                                qsTr("%1 min").arg(weatherController.performanceMonitor.alertLeadTime.toFixed(1)) : "0 min"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }
                }
            }
            
            // API Keys
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Text {
                        text: qsTr("API Keys")
                        font.pixelSize: 20
                        font.bold: true
                    }
                    
                    TextField {
                        id: pirateApiKey
                        Layout.fillWidth: true
                        placeholderText: qsTr("Pirate Weather API Key")
                        echoMode: TextInput.Password
                    }
                    
                    Button {
                        text: qsTr("Save API Key")
                        onClicked: {
                            // TODO: Save API key to settings
                            // For now, this would be handled by controller
                        }
                    }
                }
            }
            
            // Cache Management
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "white"
                radius: 10
                border.color: "#e0e0e0"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15
                    
                    Text {
                        text: qsTr("Cache Management")
                        font.pixelSize: 20
                        font.bold: true
                    }
                    
                    Button {
                        text: qsTr("Clear Cache")
                        onClicked: {
                            // TODO: Clear cache
                        }
                    }
                }
            }
        }
    }
}

