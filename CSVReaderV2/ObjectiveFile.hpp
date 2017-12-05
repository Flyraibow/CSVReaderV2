//
//  objectiveClass.h
//  CVSReader
//
//  Created by Yujie Liu on 11/13/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#ifndef objectiveClass_h
#define objectiveClass_h

#include <vector>
#include <ostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <assert.h>

using namespace std;

class ObjectiveType
{
private:
  string _typeName;
  bool _isPointer;
public:
  ObjectiveType(const string &typeName = "id", bool isPointer = false);
  ObjectiveType(const ObjectiveType &other);
  ObjectiveType& operator=(const ObjectiveType &other);
  string str() const;
  string type() const;
};

class ObjectiveProperty
{
private:
  string _propertyName;
  ObjectiveType *_propertyType;
  bool _readOnly;
  bool _atomic;
public:
  ObjectiveProperty(string propertyName, ObjectiveType *propertyType, bool readOnly = true, bool atomic = false);
  string defineContent() const;
  string simpleDefine() const;
  string staticDefine() const;
  string name() const;
  string type() const;
};

class ObjectiveFunction
{
private:
  string _funcName;
  ObjectiveType _returnType;
  vector<string> _parameterTags;
  vector<string> _parameterNames;
  vector<ObjectiveType> _parameterTypes;
  vector<string> _lines;
  bool _isStatic;
  bool _simpleWay;
public:
  ObjectiveFunction(const string &funcName);
  ObjectiveFunction(const string &funcName,
                    const ObjectiveType returnType,
                    const vector<string> &paramterTags,
                    const vector<string> &paramterNames,
                    const vector<ObjectiveType> &parameterTypes,
                    bool isStatic = false);
  
  void addLines(const string &line);
  string defineContent() const;
  string headerContent() const;
  string mainContent() const;
};

class StaticCPPFunction
{
private:
  string _funcName;
  vector<string> _lines;
public:
  StaticCPPFunction(const string &funcName);
  void addLines(const string &line);
  string mainContent() const;
};

class ObjectiveClass
{
private:
  string _className;
  vector<ObjectiveProperty *> _propertyList;
  vector<ObjectiveProperty *> _impPropertyList;
  vector<ObjectiveFunction *> _functionList;
  
public:
  ObjectiveClass(const string &className);
  void addProperty(ObjectiveProperty *property);
  void addImpProperty(ObjectiveProperty *property);
  void addFunction(ObjectiveFunction *objectiveFunction);
  
  string headerContent() const;
  string mainContent() const;
  string className() const;
};

class ObjectiveFile {
private:
  string _fileName;
  vector<string> _importHeaders;
  vector<string> _impImportHeaders;
  vector<ObjectiveClass *> _classList;
  vector<ObjectiveProperty *> _staticPropertyList;
  vector<StaticCPPFunction *> _staticCPPList;
  string _generateHFile() const;
  string _generateMFile() const;
public:
  ObjectiveFile(string fileName);
  void addImportHeaders(const string &header);
  void addImpImportHeaders(const string &header);
  void addImportFile(const ObjectiveFile &file);
  void addStaticProperty(ObjectiveProperty *property);
  void addClass(ObjectiveClass *objectiveClass);
  void addStaticCPPFunction(StaticCPPFunction *staticCppFunction);
  void writeToFile(string basePath);
  string fileName() const;
  ObjectiveClass *searchClassByName(const string className);
  friend ostream& operator<<(std::ostream& os, const ObjectiveFile &obj);
};

#endif /* objectiveClass_h */

