//
//  ReadTxtFile.cpp
//  CSVReaderV2
//
//  Created by Yujie Liu on 11/15/17.
//  Copyright © 2017 Yujie Liu. All rights reserved.
//

#include "ReadTxtFile.hpp"
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <assert.h>
#include <regex>

vector<string> getTXTFileList(const string &path) {
  vector<string> result;
  string postFix = ".txt";
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

void readFile(string basePath, string fileName, map<string, string> &localizedMap)
{
  string txtEXT = ".txt";
  string name = fileName.substr(0, fileName.length() - txtEXT.length());
  cout << name << endl;
  string fullPath = basePath + fileName;
  vector<string> propertyList;
  vector<string> propertyTypeList;
  ifstream indata;
  indata.open(fullPath);
  string cell;
  int lineNumber = 1;
  string prefix = name + "_";
  smatch match;
  regex e("\"([^\"]+)\"[ ]*=[ ]*\"([^\"]+)\";");
  bool typeSame = false;
  while (getline(indata , cell, '\n')) {
    string key;
    string value;
    cell.erase(std::remove(cell.begin(), cell.end(), '\r'), cell.end());
    if (regex_search(cell, match, e)) {
      typeSame = true;
      if (match.size() == 3) {
        key = match[1];
        value = match[2];
      }
    } else if (cell.size() > 0){
      assert(!typeSame);
      key = prefix + to_string(lineNumber);
      value = cell;
    } else {
      continue;
    }
    assert(!localizedMap.count(key));
    localizedMap[key] = value;
    lineNumber++;
  };
}
