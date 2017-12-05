//
//  ReadCSVFile.cpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/13/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#include "ReadCSVFile.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sstream>
#include <regex>
#include <ostream>
#include <unordered_map>
#include <algorithm>

using namespace std;

typedef const unordered_map<string, string> static_map;

vector<string> splitStr(const string &str, const string &token) {
  regex e(token);
  sregex_token_iterator iter(str.begin(),
                             str.end(),
                             e,
                             -1);
  vector<string> res;
  for ( ; iter != sregex_token_iterator(); ++iter) {
    res.push_back(*iter);
  }
  return res;
}

string nameChange(string originName,NameType type)
{
  string bigname = originName;
  bigname[0] = toupper(bigname[0]);
  if (type == Uppercase) {
    return bigname;
  } else if (type == DataDeclare) {
    return bigname + "Data";
  } else if (type == DataDicDeclare) {
    return bigname + "Dic";
  } else if (type == DataDicImp) {
    return "_" + originName + "Dic";
  } else if (type == DataImp) {
    return "_" + originName + "Data";
  } else if (type == GetOriginFunc) {
    return "get" + bigname;
  }
  return originName;
}

string _stripToken(const string &token)
{
  regex e("[^!-~ ]+");
  return regex_replace (token,e,"");
}

string _stripTokenToNum(const string &token)
{
  regex e("[^0-9]+");
  return regex_replace (token,e,"");
}

ObjectiveType *_getPropertyType(const string &propertyType)
{
  static const unordered_set<string> definedType({"int", "long", "double","BOOL", "NSInteger"});
  static const unordered_set<string> definedPointerType({"NSString", "NSSet"});
  static static_map definedMapType({{"string","NSString"}, {"id", "NSString"},{"groupId", "NSString"}, {"bool" , "BOOL"}, {"stringId", "NSString"}, {"set", "NSSet"}});
  if (definedMapType.count(propertyType)) {
    return _getPropertyType(definedMapType.at(propertyType));
  } else if (definedType.count(propertyType)) {
    return new ObjectiveType(propertyType);
  } else if (definedPointerType.count(propertyType)) {
    return new ObjectiveType(propertyType, true);
  }
  return nullptr;
}

string _getReadBufferByType(ObjectiveProperty *property)
{
  static static_map definedMapType({{"int", "readInt"},
    {"NSInteger", "readLong"},
    {"double", "readDouble"},
    {"long", "readLong"},
    {"NSString", "readString"},
    {"NSSet", "readSet"},
  });
  assert(definedMapType.count(property->type()));
  string result = "\t";
  result += "_" + property->name() + " = [buffer " + definedMapType.at(property->type()) + "];";
  return result;
}

vector<string> getCSVFileList(const string &path) {
  vector<string> result;
  string postFix = CSV_EXTENSION;
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (path.c_str())) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
      char *chr = ent->d_name;
      size_t len =strlen(chr);
      size_t postLen = postFix.length();
      if (len > postLen && strcmp(postFix.c_str(),chr + len - postLen) == 0) {
        string fullName = chr;
        result.push_back(fullName);
      }
    }
    closedir (dir);
  } else {
    /* could not open directory */
    perror ("");
    cout<< " ERROR " << endl;
    return result;
  }
  return result;
}

void _saveBuffer(unique_ptr<bb::ByteBuffer> &buffer, ObjectiveType *type, string &token)
{
  if (type->type() == "int") {
    buffer->putInt(stoi(token));
  } else if (type->type() == "long" || type->type() == "NSInteger") {
    buffer->putLong(stoll(token));
  } else if (type->type() == "double") {
    buffer->putDouble(stod(token));
  } else if (type->type() == "BOOL") {
    buffer->putInt(stoi(token));
  } else if (type->type() == "NSString") {
    buffer->putString(token);
  } else if (type->type() == "NSSet" || type->type() == "set") {
    vector<string> sets = splitStr(token, ";");
    buffer->putLong(sets.size());
    for (int i = 0; i < sets.size(); ++i) {
      buffer->putString(sets[i]);
    }
  } else {
    throw new exception;
  }
}

ObjectiveClass *_prepareDicClass(const string &name, const string &key, bool isGroup)
{
  ObjectiveClass *objectiveDic = new ObjectiveClass(nameChange(name, DataDicDeclare));
  ObjectiveFunction *initWithByteBuffer = new ObjectiveFunction("-(instancetype)initWithByteBuffer:(ByteBuffer *)buffer");
  string dataDeclare = nameChange(name, DataDeclare);
  initWithByteBuffer->addLines("self = [self init];");
  initWithByteBuffer->addLines("if (self) {");
  initWithByteBuffer->addLines("\tint amount = [buffer readInt];");
  if (isGroup) {
    initWithByteBuffer->addLines("\t_groupData = [NSMutableDictionary new];");
  } else {
    initWithByteBuffer->addLines("\t_data = [NSMutableDictionary new];");
  }
  initWithByteBuffer->addLines("\tfor (int i = 0; i < amount; ++i) {");
  initWithByteBuffer->addLines("\t\t" + dataDeclare + " *data = [[" + dataDeclare + " alloc] initWithByteBuffer:buffer];");
  if (isGroup) {
    initWithByteBuffer->addLines("\t\tif ([_groupData objectForKey:data." + key + "] == nil) {");
    initWithByteBuffer->addLines("\t\t\t[_groupData setObject:[NSMutableArray new] forKey:data." + key + "];");
    initWithByteBuffer->addLines("\t\t}");
    initWithByteBuffer->addLines("[[_groupData objectForKey:data."+ key + "] addObject:data];");
  } else {
    initWithByteBuffer->addLines("\t\t[_data setObject:data forKey:data." + key +"];");
  }
  initWithByteBuffer->addLines("\t}");
  initWithByteBuffer->addLines("}");
  initWithByteBuffer->addLines("return self;");
  objectiveDic->addFunction(initWithByteBuffer);
  
  string dataUpper = nameChange(name, Uppercase);
  ObjectiveFunction *getDataById;
  if (isGroup) {
    getDataById = new ObjectiveFunction("-(NSArray *)get" + dataUpper + "GroupByGroupId:(NSString *)groupId");
    getDataById->addLines("return [_groupData objectForKey:groupId];");
    ObjectiveProperty *groupData = new ObjectiveProperty("_groupData", new ObjectiveType("NSMutableDictionary", true));
    objectiveDic->addImpProperty(groupData);
  } else {
    getDataById = new ObjectiveFunction("-(" + dataDeclare + " *)get" + dataUpper + "ById:(NSString *)" + key);
    getDataById->addLines("return [_data objectForKey:" + key + "];");
    ObjectiveFunction *getDictionary = new ObjectiveFunction("-(NSDictionary *)getDictionary");
    getDictionary->addLines("return _data;");
    objectiveDic->addFunction(getDictionary);
    ObjectiveProperty *data = new ObjectiveProperty("_data", new ObjectiveType("NSMutableDictionary", true));
    objectiveDic->addImpProperty(data);
  }
  objectiveDic->addFunction(getDataById);
  
  return objectiveDic;
}

ObjectiveFile* convertCSVToObjectiveClass(const string &basePath,
                                          const string &fileName,
                                          const string &savePath,
                                          map<string, string> &localizedMap,
                                          unique_ptr<bb::ByteBuffer> &buffer)
{
  string name = fileName.substr(0, fileName.length() - CSV_EXTENSION.length());
  cout << name << endl;

  ObjectiveFile *objectiveFile = new ObjectiveFile(nameChange(name, DataDeclare));
  ObjectiveClass *objectiveData = new ObjectiveClass(nameChange(name, DataDeclare));
  objectiveFile->addClass(objectiveData);
  objectiveFile->addImportHeaders("\"ByteBuffer.h\"");
  ObjectiveType initType("instancetype");
  ObjectiveType bufferType("ByteBuffer", true);
  ObjectiveFunction *initWithByteBufferFunction = new ObjectiveFunction("initWithByteBuffer",
                                                                        initType,
                                                                        vector<string>(),
                                                                        vector<string>({"buffer"}),
                                                                        vector<ObjectiveType>({bufferType}));
  
  initWithByteBufferFunction->addLines("self = [self init];");
  initWithByteBufferFunction->addLines("if (self) {");
  objectiveData->addFunction(initWithByteBufferFunction);
  
  string fullPath = basePath + fileName;
  vector<string> propertyList;
  vector<string> propertyTypeList;
  ifstream indata;
  indata.open(fullPath);
  string cell;
  
  int lineNumber = -2;
  int idIndex = -1;
  string key;
  unordered_set<string> stringIdSet;
  // put temporary 0 for replacement in the future
  uint32_t pos = buffer->size();
  buffer->putInt(0);
  bool isGroup = false;
  const static string namePrefix = "name";
  string idIvar;
  while (getline(indata , cell, '\r')) {
    cell.erase(std::remove(cell.begin(), cell.end(), '\n'), cell.end());
    istringstream ss(cell);
    regex e("[\t,]");
    sregex_token_iterator iter(cell.begin(),
                               cell.end(),
                               e,
                               -1);
    int colNum = 0;
    string id_string;
    for ( ; iter != sregex_token_iterator(); ++iter) {
      string token = *iter;
      if (lineNumber == -2) {
        token = _stripToken(token);
        propertyList.push_back(token);
      } else if (lineNumber == -1) {
        token = _stripToken(token);
        propertyTypeList.push_back(token);
        if (!token.compare(0, namePrefix.size(), namePrefix) && token.size() > namePrefix.size() + 1) {
          string stringFuncName = token.substr(namePrefix.size() + 1);
          string prefixString = propertyList[propertyTypeList.size() - 1];
          assert(idIvar.size() > 0);
          string getStringFuncName = nameChange(stringFuncName, GetOriginFunc);
          StaticCPPFunction *staticCppFunction = new StaticCPPFunction("static NSString * " + getStringFuncName + "(NSString *" + idIvar +")");
          staticCppFunction->addLines("NSString *string = [NSString stringWithFormat:@\"" + prefixString + "_%@\", " + idIvar + "];");
          staticCppFunction->addLines("return NSLocalizedString(string, nil);");
          objectiveFile->addStaticCPPFunction(staticCppFunction);
          ObjectiveFunction *getStringFunction = new ObjectiveFunction("-(NSString *)" + stringFuncName);
          getStringFunction->addLines("return " + getStringFuncName + "(_" + idIvar + ");" );
          objectiveData->addFunction(getStringFunction);
        } else if (token == "id" || token == "stringId") {
          idIndex = (int)propertyTypeList.size() - 1;
          idIvar = propertyList[idIndex];
        }
      } else {
        if (colNum == idIndex) {
          id_string = token;
          if (!isGroup) {
            assert(!stringIdSet.count(token));
            stringIdSet.insert(token);
          }
        } else {
          string propertyType = propertyTypeList[colNum];
          if (!propertyType.compare(0, namePrefix.size(), namePrefix)) {
            assert(id_string.size() > 0);
            string propertyName = id_string;
            if (propertyList[colNum] != ";") {
              propertyName = propertyList[colNum] + "_" + id_string;
            }
            localizedMap[propertyName] = token;
          }
        }
        string propertyType = propertyTypeList[colNum];
        ObjectiveType *objectiveType = _getPropertyType(propertyType);
        if (objectiveType != nullptr) {
          _saveBuffer(buffer, objectiveType, token);
        }
        colNum++;
        if (colNum > propertyList.size()) {
          break;
        }
      }
    }
    if (lineNumber >= 0) {
      assert(colNum == propertyList.size());
    }
    
    if (++lineNumber == 0) {
      bool containIDorGroupId = false;
      assert(propertyList.size() == propertyTypeList.size());
      for (int i = 0; i < propertyList.size(); ++i) {
        string propertyName = propertyList[i];
        string propertyType = propertyTypeList[i];
        ObjectiveType *objectiveType = _getPropertyType(propertyType);
        if (objectiveType != nullptr) {
          if (propertyType == "id" || propertyType == "stringId" || propertyType == "groupId") {
            isGroup = (propertyType == "groupId");
            key = propertyName;
            assert(!containIDorGroupId);
            containIDorGroupId = true;
            idIndex = i;
          }
          ObjectiveProperty *objectiveProperty = new ObjectiveProperty(propertyName, objectiveType);
          objectiveData->addProperty(objectiveProperty);
          initWithByteBufferFunction->addLines(_getReadBufferByType(objectiveProperty));
        }
      }
      assert(containIDorGroupId);
      
      initWithByteBufferFunction->addLines("}");
      initWithByteBufferFunction->addLines("return self;");
    }
  }
  ObjectiveClass *objectiveDic = _prepareDicClass(name, key, isGroup);
  objectiveFile->addClass(objectiveDic);
  objectiveFile->writeToFile(savePath);
  buffer->putInt(lineNumber, pos);
  buffer->setWritePos(buffer->size());
  return objectiveFile;
}


ObjectiveFile* convertMatriceCSVToObjectiveClass(const string &basePath,
                                                 const string &fileName,
                                                 const string &savePath,
                                                 unique_ptr<bb::ByteBuffer> &buffer)
{
  string name = fileName.substr(0, fileName.length() - CSV_EXTENSION.length());
  cout << name << endl;
  string fullPath = basePath + fileName;
  ifstream indata;
  indata.open(fullPath);
  string cell;
  
  ObjectiveFile *objectiveFile = new ObjectiveFile(nameChange(name, DataDeclare));
  // 第一行，第一列是ID， 第二列是说明，后面是数据
  int row = 0;
  string functionName;
  string rowName;
  string colName;
  string valueType;
  vector<string> rowValues;
  vector<string> colValues;
  unordered_set<string> rowSet;
  unordered_set<string> colSet;
  vector<vector<string>> values;
  while (getline(indata , cell, '\r')) {
    cell.erase(std::remove(cell.begin(), cell.end(), '\n'), cell.end());
    istringstream ss(cell);
    regex e("[\t,]");
    sregex_token_iterator iter(cell.begin(),
                               cell.end(),
                               e,
                               -1);
    int col = 0;
    vector<string> list;
    for ( ; iter != sregex_token_iterator(); ++iter) {
      string token = *iter;
      if (row == 0) {
        if (col == 0) {
          functionName = _stripToken(token);
        } else if (col == 1) {
          colName = token;
        } else {
          assert(!colSet.count(token));
          colSet.insert(token);
          colValues.push_back(token);
        }
      } else if (row == 1) {
        if (col == 0) {
          rowName = token;
        } else if (col == 1) {
          valueType = token;
        }
      } else {
        if (col == 0) {
          assert(!rowSet.count(token));
          rowSet.insert(token);
          rowValues.push_back(token);
        } else if (col > 1) {
          list.push_back(token);
        }
      }
      col++;
    }
    if (row > 1) {
      values.push_back(list);
    }
    row++;
  }
  //validate
  assert(values.size() == rowValues.size());
  for (int i = 0; i < values.size(); ++i) {
    vector<string> list = values[i];
    assert(list.size() == colValues.size());
  }
  buffer->putLong(colValues.size());
  for (int i = 0; i < colValues.size(); ++i) {
    buffer->putString(colValues[i]);
  }
  buffer->putLong(rowValues.size());
  for (int i = 0; i < rowValues.size(); ++i) {
    buffer->putString(rowValues[i]);
  }
  for (int i = 0; i < values.size(); ++i) {
    vector<string> list = values[i];
    for (int j = 0; j < list.size(); ++j) {
      string value = list[j];
      buffer->putString(value);
    }
  }
  
  ObjectiveClass *objectiveData = new ObjectiveClass(nameChange(name, DataDeclare));
  objectiveFile->addClass(objectiveData);
  objectiveFile->addImportHeaders("\"ByteBuffer.h\"");
  objectiveData->addImpProperty(new ObjectiveProperty("_dictionary", new ObjectiveType("NSMutableDictionary", true)));
  ObjectiveFunction *initWithByteBufferFunction = new ObjectiveFunction("-(instancetype )initWithByteBuffer:(ByteBuffer *)buffer");
  initWithByteBufferFunction->addLines("self = [self init];");
  initWithByteBufferFunction->addLines("if (self) {");
  initWithByteBufferFunction->addLines("\t_dictionary = [NSMutableDictionary new];");
  initWithByteBufferFunction->addLines("\tNSInteger colCount = [buffer readLong];");
  initWithByteBufferFunction->addLines("\tNSMutableArray *colValues = [NSMutableArray new];");
  initWithByteBufferFunction->addLines("\tfor (int i = 0; i < colCount; ++i) {");
  initWithByteBufferFunction->addLines("\t\t[colValues addObject:[buffer readString]];");
  initWithByteBufferFunction->addLines("\t}");
  initWithByteBufferFunction->addLines("\tNSInteger rowCount = [buffer readLong];");
  initWithByteBufferFunction->addLines("\tNSMutableArray *rowValues = [NSMutableArray new];");
  initWithByteBufferFunction->addLines("\tfor (int i = 0; i < rowCount; ++i) {");
  initWithByteBufferFunction->addLines("\t\t[rowValues addObject:[buffer readString]];");
  initWithByteBufferFunction->addLines("\t}");
  initWithByteBufferFunction->addLines("\tfor (int i = 0; i < rowCount; ++i) {");
  initWithByteBufferFunction->addLines("\t\tNSMutableDictionary *dict = [NSMutableDictionary new];");
  initWithByteBufferFunction->addLines("\t\tfor (int j = 0; j < colCount; ++j) {");
  initWithByteBufferFunction->addLines("\t\t\tNSString *value = [buffer readString];");
  initWithByteBufferFunction->addLines("\t\t\tdict[colValues[j]] = value;");
  initWithByteBufferFunction->addLines("\t\t}");
  initWithByteBufferFunction->addLines("\t\t_dictionary[rowValues[i]] = dict;");
  initWithByteBufferFunction->addLines("\t}");
  initWithByteBufferFunction->addLines("}");
  initWithByteBufferFunction->addLines("return self;");
  
  ObjectiveType *returnType = _getPropertyType(valueType);
  if (returnType == nullptr) {
    returnType = new ObjectiveType("NSString", true);
  }
  ObjectiveType stringType("NSString",true);
  string getFunctionName = functionName + "By" + nameChange(colName, Uppercase);
  ObjectiveFunction *getDataFunction = new ObjectiveFunction(getFunctionName,
                                                             *returnType,
                                                             vector<string>{rowName},
                                                             vector<string>{colName,rowName},
                                                             vector<ObjectiveType>({stringType, stringType})
                                                             );
  getDataFunction->addLines("NSString *value = _dictionary[" + rowName + "][" + colName + "];");
  if (valueType == "int") {
    getDataFunction->addLines("int val = [value intValue];");
  } else if (valueType == "long" || valueType == "NSInteger") {
    getDataFunction->addLines("NSInteger val = [value integerValue];");
  } else if (valueType == "double") {
    getDataFunction->addLines("double val = [value doubleValue];");
  } else {
    getDataFunction->addLines("NSString *val = value;");
  }
  getDataFunction->addLines("return val;");
  objectiveData->addFunction(initWithByteBufferFunction);
  objectiveData->addFunction(getDataFunction);
  
  
  objectiveFile->writeToFile(savePath);
  
  return objectiveFile;
}
