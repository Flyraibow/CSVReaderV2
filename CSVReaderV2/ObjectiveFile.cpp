//
//  ObjectiveFile.cpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/14/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#include "ObjectiveFile.hpp"

using namespace std;

ObjectiveType::ObjectiveType(const string &typeName, bool isPointer) {
  _typeName = typeName;
  _isPointer = isPointer;
}

ObjectiveType::ObjectiveType(const ObjectiveType &other) {
  _typeName = other._typeName;
  _isPointer = other._isPointer;
}

ObjectiveType& ObjectiveType::operator=(const ObjectiveType &other)
{
  if (this != &other) {
    _typeName = other._typeName;
    _isPointer = other._isPointer;
  }
  return *this;
}

string ObjectiveType::str() const {
  if (_isPointer) {
    return _typeName + " *";
  }
  return _typeName + " ";
}

string ObjectiveType::type() const {
  return _typeName;
}

ObjectiveProperty::ObjectiveProperty(string propertyName, ObjectiveType *propertyType, bool readOnly, bool atomic) {
  assert(propertyType != nullptr);
  _propertyName = propertyName;
  _propertyType = propertyType;
  _readOnly = readOnly;
  _atomic = atomic;
}

string ObjectiveProperty::defineContent() const {
  string file = "@property (" ;
  file += _atomic ? "atomic" : "nonatomic";
  file += ",";
  file += _readOnly ? "readonly" : "readwrite";
  file += ") ";
  file += simpleDefine();
  return file;
}

string ObjectiveProperty::simpleDefine() const {
  string file;
  file += _propertyType->str();
  file += _propertyName;
  file += ";";
  return file;
}

string ObjectiveProperty::staticDefine() const {
  return "static " + simpleDefine();
}

string ObjectiveProperty::name() const {
  return _propertyName;
}
string ObjectiveProperty::type() const {
  return _propertyType->type();
}


ObjectiveFunction::ObjectiveFunction(string funcName) {
  _funcName = funcName;
  _simpleWay = true;
}

ObjectiveFunction::ObjectiveFunction(string funcName,
                                     const ObjectiveType returnType,
                                     const vector<string> &paramterTags,
                                     const vector<string> &paramterNames,
                                     const vector<ObjectiveType> &parameterTypes,
                                     bool isStatic) {
  _funcName = funcName;
  _returnType = returnType;
  assert(paramterNames.size() == parameterTypes.size());
  assert(paramterNames.size() == 0 || paramterTags.size() + 1 == paramterNames.size());
  _parameterTags = paramterTags;
  _parameterNames = paramterNames;
  _parameterTypes = parameterTypes;
  _isStatic = isStatic;
  _simpleWay = false;
}

void ObjectiveFunction::addLines(const string &line) {
  _lines.push_back(line);
}

string ObjectiveFunction::defineContent() const {
  
  string content;
  if (_simpleWay) {
    content = _funcName;
  } else {
    content = _isStatic ? "+" : "-";
    content += "(" + _returnType.str() + ")" + _funcName;
  }
  if (_parameterTypes.size() > 0) {
    for (int i = 0; i < _parameterTypes.size(); ++i) {
      if (i > 0) {
        content += " " + _parameterTags[i - 1];
      }
      content += ":(" + _parameterTypes[i].str() + ")" + _parameterNames[i];
    }
  }
  return content;
}

string ObjectiveFunction::headerContent() const {
  string content = defineContent();
  return content + ";";
}

string ObjectiveFunction::mainContent() const {
  string content = defineContent();
  content += "\n{\n";
  for (int i = 0; i < _lines.size(); ++i) {
    content += "\t" + _lines[i] + "\n";
  }
  content += "}";
  return content;
}


ObjectiveClass::ObjectiveClass(const string &className) {
  _className = className;
}

void ObjectiveClass::addProperty(ObjectiveProperty *property) {
  _propertyList.push_back(property);
}

void ObjectiveClass::addImpProperty(ObjectiveProperty *property) {
  _impPropertyList.push_back(property);
}

void ObjectiveClass::addFunction(ObjectiveFunction *objectiveFunction) {
  _functionList.push_back(objectiveFunction);
}

string ObjectiveClass::headerContent() const {
  string content = "@interface " + _className + " : NSObject\n";
  content += "\n";
  for (ObjectiveProperty *property : _propertyList) {
    content += property->defineContent() + "\n";
    content += "\n";
  }
  for (ObjectiveFunction *oFunction : _functionList) {
    content += oFunction->defineContent() + ";\n";
    content += "\n";
  }
  content += "@end\n";
  return content;
}

string ObjectiveClass::mainContent() const {
  string content = "@implementation " + _className + "\n";
  if (_impPropertyList.size() > 0) {
    content += "{\n";
    for (ObjectiveProperty *property : _impPropertyList) {
      content += "\t" + property->simpleDefine() + "\n";
    }
    content += "}\n";
  }
  for (ObjectiveFunction *oFunction : _functionList) {
    content += oFunction->mainContent() + "\n";
  }
  content += "\n@end\n";
  return content;
}

string ObjectiveClass::className() const
{
  return _className;
}

string ObjectiveFile::_generateHFile() const {
  string file = "/* This file is generated, do not modify it !*/\n";
  file += "\n";
  for (string header : _importHeaders) {
    file += "#import " + header + "\n";
  }
  file += "\n";
  for (ObjectiveClass *oc : _classList) {
    file += oc->headerContent() + "\n";
  }
  return file;
}

string ObjectiveFile::_generateMFile() const {
  string file = "/* This file is generated, do not modify it !*/\n";
  file += "#import \"" + _fileName + ".h\"\n";
  for (string header : _impImportHeaders) {
    file += "#import " + header + "\n";
  }
  if (_staticPropertyList.size() > 0) {
    file += "\n";
    for (ObjectiveProperty *property : _staticPropertyList) {
      file += property->staticDefine() + "\n";
    }
    file += "\n";
  }
  for (ObjectiveClass *oc : _classList) {
    file += oc->mainContent() + "\n";
  }
  return file;
}

ObjectiveFile::ObjectiveFile(string fileName) {
  _fileName = fileName;
  assert(_fileName.length() > 0);
  addImportHeaders("<Foundation/Foundation.h>");
}
void ObjectiveFile::addImportHeaders(const string &header) {
  _importHeaders.push_back(header);
}

void ObjectiveFile::addImpImportHeaders(const string &header) {
  _impImportHeaders.push_back(header);
}

void ObjectiveFile::addImportFile(const ObjectiveFile &file) {
  _importHeaders.push_back("\"" + file._fileName + ".h\"");
}

void ObjectiveFile::addStaticProperty(ObjectiveProperty *property) {
  _staticPropertyList.push_back(property);
}

void ObjectiveFile::addClass(ObjectiveClass *objectiveClass) {
  _classList.push_back(objectiveClass);
}

void ObjectiveFile::writeToFile(string basePath) {
  string headerPath = basePath + "/" + _fileName + ".h";
  string dataH = _generateHFile();
  ofstream dataHFile(headerPath);
  dataHFile << dataH;
  dataHFile.close();
  
  string contentPath = basePath + "/" + _fileName + ".m";
  string dataM = _generateMFile();
  ofstream dataMFile(contentPath);
  dataMFile << dataM;
  dataMFile.close();
}

string ObjectiveFile::fileName() const
{
  return _fileName;
}

ObjectiveClass *ObjectiveFile::searchClassByName(const string className)
{
  for (int i = 0; i < _classList.size(); ++i) {
    if (_classList[i]->className() == className) {
      return _classList[i];
    }
  }
  return nullptr;
}

ostream& operator<<(std::ostream& os, const ObjectiveFile &obj)
{
  os << obj._fileName << endl;
  return os;
}

