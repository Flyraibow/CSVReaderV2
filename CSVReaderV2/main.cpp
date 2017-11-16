//
//  main.cpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/13/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#include <iostream>
#include <regex>
#include <map>
#include "ByteBuffer.hpp"
#include "ObjectiveFile.hpp"
#include "ReadCSVFile.hpp"
#include "ReadTxtFile.hpp"

using namespace std;

void _prepareDataManager(ObjectiveClass *dataManagerClass)
{
  ObjectiveFunction *dataManagerWithData = new ObjectiveFunction("+(instancetype)dataManagerWithData:(NSData *)data");
  dataManagerWithData->addLines("if (_sharedDataManager == nil) {");
  dataManagerWithData->addLines("\t_sharedDataManager = [[DataManager alloc] initWithData:data];");
  dataManagerWithData->addLines("}");
  dataManagerWithData->addLines("return _sharedDataManager;");
  dataManagerClass->addFunction(dataManagerWithData);
  
  ObjectiveFunction *sharedDataManager = new ObjectiveFunction("+(DataManager *)sharedDataManager");
  sharedDataManager->addLines("return _sharedDataManager;");
  dataManagerClass->addFunction(sharedDataManager);
}

void _prepareDataManager(const string &fullName,
                         ObjectiveClass *dataManagerClass,
                         ObjectiveFunction *dataManagerInitFunction,
                         bool isMatrix)
{
  string name = fullName.substr(0, fullName.length() - CSV_EXTENSION.length());
  string dicString = isMatrix ? nameChange(name,DataDeclare) : nameChange(name, DataDicDeclare);
  string dicImpString = isMatrix ? nameChange(name, DataImp) : nameChange(name, DataDicImp);
  ObjectiveProperty *impProperty = new ObjectiveProperty(dicImpString, new ObjectiveType(dicString, true));
  dataManagerClass->addImpProperty(impProperty);
  dataManagerInitFunction->addLines("\t" + dicImpString + " = [[" + dicString + " alloc] initWithByteBuffer:buffer];");
  ObjectiveFunction *getDicFunction = new ObjectiveFunction("-(" + dicString + " *)get" + dicString);
  getDicFunction->addLines("return " + dicImpString + ";");
  dataManagerClass->addFunction(getDicFunction);
}

int main(int argc, const char * argv[]) {
  // insert code here...
  string rootPath = "/Users/yujiel/Dropbox (Personal)/Code/gitCode/SalingGame/";
  string resourcePath = rootPath + "FileTestProject/Resources/";
  string classPath = rootPath + "FileTestProject/Classes/DataProcessing/";
  string inputPath = rootPath + "DataSource/excel/";
  string inputPath2 = rootPath + "DataSource/Matrix/";
  string inputPath3 = rootPath + "DataSource/string/";
  string localStringPath = resourcePath + "Localizable.strings";
  
  auto buffer = std::make_unique<bb::ByteBuffer>();
  static const string DataManager = "DataManager";
  ObjectiveFile *dataManagerFile = new ObjectiveFile(DataManager);
  ObjectiveClass *dataManagerClass = new ObjectiveClass(DataManager);
  dataManagerFile->addClass(dataManagerClass);
  ObjectiveType *dataManagerType = new ObjectiveType(DataManager, true);
  ObjectiveProperty *staticProperty = new ObjectiveProperty("_sharedDataManager", dataManagerType);
  dataManagerFile->addStaticProperty(staticProperty);
  ObjectiveFunction *dataManagerInitFunction = new ObjectiveFunction("-(instancetype)initWithData:(NSData *)data");
  dataManagerClass->addFunction(dataManagerInitFunction);
  dataManagerInitFunction->addLines("self = [self init];");
  dataManagerInitFunction->addLines("if (self) {");
  dataManagerInitFunction->addLines("\tByteBuffer *buffer = [[ByteBuffer alloc] initWithData:data];");
  _prepareDataManager(dataManagerClass);
  
  // load normal csv file
  map<string, string> localizedMap;
  vector<string> fileList = getCSVFileList(inputPath);
  for (int i = 0; i < fileList.size(); ++i) {
    string fullName = fileList[i];
    ObjectiveFile *file = convertCSVToObjectiveClass(
                                                     inputPath,
                                                     fullName,
                                                     classPath,
                                                     localizedMap,
                                                     buffer);
    dataManagerFile->addImportFile(*file);
    _prepareDataManager(fullName, dataManagerClass, dataManagerInitFunction, false);
  }
  // load matrice csv file
  fileList = getCSVFileList(inputPath2);
  for (int i = 0; i < fileList.size(); ++i) {
    string fullName = fileList[i];
    ObjectiveFile *file = convertMatriceCSVToObjectiveClass(inputPath2,
                                                            fullName,
                                                            classPath,
                                                            buffer);
    dataManagerFile->addImportFile(*file);
    _prepareDataManager(fullName, dataManagerClass, dataManagerInitFunction, true);
  }
  
  dataManagerInitFunction->addLines("}");
  dataManagerInitFunction->addLines("return self;");
  dataManagerFile->writeToFile(classPath);
  
  // write buffer
  auto size = buffer->size();
  cout << size << endl;
  ofstream outfile (resourcePath + "game.dat",ofstream::binary);
  uint8_t* buf = new uint8_t[size];
  buffer->getBytes(buf, size);
  char *chrs = new char[size];
  memcpy(chrs, buf, size);
  outfile.write (chrs,size);
  delete [] buf;
  delete [] chrs;
  
  // read extra string file
  
  fileList = getTXTFileList(inputPath3);
  for (int i = 0; i < fileList.size(); ++i) {
    string fullName = fileList[i];
    readFile(inputPath3, fullName, localizedMap);
  }
  
  // write local string
  ofstream localStringStream(localStringPath);
  for (pair<string, string> element : localizedMap)
  {
    regex e("\"");
    string key = regex_replace(element.first, e, "\\\"");
    string value = regex_replace(element.second, e, "\\\"");
    string content = "\"" + key + "\" = \"" + value + "\";\n";
    localStringStream << content;
  }
  localStringStream.close();
  
  return 0;
}
