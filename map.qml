import QtQuick 2.12
import QtPositioning 5.12
//import QtLocation 5.12
//! [Imports]

Rectangle{}

//Rectangle {
//    anchors.fill: parent
//
//    //! [Initialize Plugin]
//    Plugin {
//        id: myPlugin
//        name: "osm" // "mapboxgl", "esri", ...
//        //specify plugin parameters if necessary
//        //PluginParameter {...}
//        //PluginParameter {...}
//        //...
//    }
//    //! [Initialize Plugin]
//
//    //! [Current Location]
//    PositionSource {
//        id: positionSource
//        property variant lastSearchPosition: locationOslo
//        active: true
//        updateInterval: 120000 // 2 mins
//        onPositionChanged:  {
//            var currentPosition = positionSource.position.coordinate
//            map.center = currentPosition
//            var distance = currentPosition.distanceTo(lastSearchPosition)
//            if (distance > 500) {
//                // 500m from last performed pizza search
//                lastSearchPosition = currentPosition
//                searchModel.searchArea = QtPositioning.circle(currentPosition)
//                searchModel.update()
//            }
//        }
//    }
//    //! [Current Location]
//
//    //! [PlaceSearchModel]
//    property variant locationOslo: QtPositioning.coordinate( 59.93, 10.76)
//
//    PlaceSearchModel {
//        id: searchModel
//
//        plugin: myPlugin
//
//        searchTerm: "Pizza"
//        searchArea: QtPositioning.circle(locationOslo)
//
//        Component.onCompleted: update()
//    }
//    //! [PlaceSearchModel]
//
//    //! [Places MapItemView]
//    Map {
//        id: map
//        anchors.fill: parent
//        plugin: myPlugin;
//        center: locationOslo
//        zoomLevel: 13
//
//        MapItemView {
//            model: searchModel
//            delegate: MapQuickItem {
//                coordinate: place.location.coordinate
//
//                anchorPoint.x: image.width * 0.5
//                anchorPoint.y: image.height
//
//                sourceItem: Column {
//                    Image { id: image; source: "marker.png" }
//                    Text { text: title; font.bold: true }
//                }
//            }
//        }
//    }
//    //! [Places MapItemView]
//
//    Connections {
//        target: searchModel
//        onStatusChanged: {
//            if (searchModel.status == PlaceSearchModel.Error)
//                console.log(searchModel.errorString());
//        }
//    }
//}
