//
//  ReadCSVFile.hpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/13/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#ifndef ReadCSVFile_hpp
#define ReadCSVFile_hpp

#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include "ObjectiveFile.hpp"
#include "ByteBuffer.hpp"

using namespace std;

const static string CSV_EXTENSION = ".csv";

enum NameType { Origin, Uppercase, DataDeclare, DataDicDeclare, DataDicImp, DataImp};

string nameChange(string originName,NameType type);

vector<string> getCSVFileList(const string &path);

ObjectiveFile* convertCSVToObjectiveClass(const string &basePath,
                                          const string &fileName,
                                          const string &savePath,
                                          map<string, string> &localizedMap,
                                          unique_ptr<bb::ByteBuffer> &buffer);

ObjectiveFile* convertMatriceCSVToObjectiveClass(const string &basePath,
                                                 const string &fileName,
                                                 const string &savePath,
                                                 unique_ptr<bb::ByteBuffer> &buffer);


#endif /* ReadCSVFile_hpp */
