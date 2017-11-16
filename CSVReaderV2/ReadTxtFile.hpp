//
//  ReadTxtFile.hpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/15/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#ifndef ReadTxtFile_hpp
#define ReadTxtFile_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <vector>

using namespace std;
vector<string> getTXTFileList(const string &path);

void readFile(string basePath, string fileName, map<string, string> &localizedMap);

#endif /* ReadTxtFile_hpp */
