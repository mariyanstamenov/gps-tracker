//
//  MapCreator.h
//  GPSTracker
//
//  Created by Manss Stamenov on 26.05.21.
//

#include <string>
#include <vector>

#ifndef MapCreator_h
#define MapCreator_h

std::string createHeader();
std::string createBody(std::string viewX, std::string viewY, std::string points, std::string markers);
std::string createFooter();

std::string getMap(std::vector<std::string> data);
#endif /* MapCreator_h */
