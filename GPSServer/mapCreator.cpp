#include "MapCreator.h"

std::string createHeader() {
    return "<html>\n"
                "\t<head>\n"
                    "\t\t<link rel='stylesheet' href='https://unpkg.com/leaflet@1.7.1/dist/leaflet.css' />\n"
                    "\t\t<script src='https://unpkg.com/leaflet@1.7.1/dist/leaflet.js'></script>\n"
                    "\t\t<style>\n"
                        "\t\t\tbody { margin: 0; }\n"
                        "\t\t\t#mapid { width: 100%; height: 100%; margin: 0px auto; }\n"
                    "\t\t</style>\n"
                "\t</head>\n";
};

std::string createBody(std::string viewX, std::string viewY, std::string points, std::string markers, std::string labels) {
    
    return "\t<body>\n"
                "\t\t<div id='mapid'></div>\n"
                "\t\t<script type='text/javascript'>\n"
                    "\t\t\tconst viewX = " + viewX + ";\n"
                    "\t\t\tconst viewY = " + viewY + ";\n"
                    "\t\t\tconst mymap = L.map('mapid').setView([viewX, viewY], 16);\n"
                    "\t\t\tL.tileLayer('https://api.mapbox.com/styles/v1/{id}/tiles/{z}/{x}/{y}?access_token=sk.eyJ1IjoibWFuc3N0YW1lbm92IiwiYSI6ImNraGFwZ3prdDE5N3YycnJ0d3J1M3J4MjEifQ.QBk3DpINDnMmCpMYgNlqjQ', {\n"
                        "\t\t\t\tmaxZoom: 18,\n"
                        "\t\t\t\tid: 'mapbox/streets-v11',\n"
                        "\t\t\t\ttileSize: 512,\n"
                        "\t\t\t\tzoomOffset: -1,\n"
                    "\t\t\t}).addTo(mymap);\n"
                    "\t\t\tconst arr = [" + points + "];\n"
                    "\t\t\tconst labels = " + labels + ";\n"
                    "\t\t\t" + markers + "\n"
                    "\t\t\tconst polyline = L.polyline(arr, { color: 'red' }).addTo(mymap);"
                "\t\t</script>\n"
            "\t</body>\n";
};

std::string createFooter() {
    return "</html>";
}

std::string getMap(std::vector<std::string> data) {
    std::string points = "";
    std::string markers = "for (let i = 0; i < arr.length; i++) {\n";
    std::string labels = "[";
    double startLat = 0.0;
    double startLng = 0.0;
    
    //std::cout << std::fixed;
    //std::cout << std::setprecision(6);
    
    for(int i = 0; i < data.size(); i++) {
        std::string _line = data.at(i);
        std::string lat = _line.substr(0, _line.find(";"));
        _line = _line.substr(_line.find(";") + 1);
        
        std::string lng = _line.substr(0, _line.find(";"));
        _line = _line.substr(_line.find(";") + 1);
        
        std::string date = _line.substr(0, _line.find(";"));
        _line = _line.substr(_line.find(";") + 1);
        
        std::string time = _line.substr(0, _line.find(";"));
        if(i == 0) {
            startLat = stod(lat);
            startLng = stod(lng);
        }
        
        if(i != 0)  {
            points += ", ";
            labels += ", ";
        }
        
        points += "[" + lat + ", " + lng + "]";
        labels += "'Date: " + date + " Time: " + time + "'";
    }
    labels += "]";
    
    markers += "\t\t\t\tconst marker = L.marker([arr[i][0], arr[i][1]], { clickable: true, }).addTo(mymap);\n";
    markers += "\t\t\t\tmarker.bindTooltip(labels[i]);\n";
    markers += "\t\t\t}\n";
    
    std::string html = "";
    html += createHeader();
    html += createBody(std::to_string(startLat), std::to_string(startLng), points, markers, labels);
    html += createFooter();
    return html;
}
