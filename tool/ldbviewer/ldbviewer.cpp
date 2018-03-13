// Copyright (c) 2009-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "leveldb/db.h"
#include "ldbdef.h"
#include "ldbobject.h"
#include "ldbparser.h"
#include "ldbjsonviewer.h"
#include "ldbheader.h"
#include <iostream>
#include <map>
#include <list>
#include <algorithm>
#include <queue>

using namespace std;

enum DB8_TYPE {
    DB8_TYPE_ALL,
    DB8_TYPE_KINDS,
    DB8_TYPE_OBJECTS,
    DB8_TYPE_INDEXIDS,
    DB8_TYPE_INDEXES,
    DB8_TYPE_SEQ,
    DB8_TYPE_CATALOG,
    DB8_TYPE_DBNAME,
};

typedef int VIEW_FLAG;
#define VIEW_FLAG_NONE       0x00000000
#define VIEW_FLAG_RAW        0x00000001
#define VIEW_FLAG_ALL        0x00000002
#define VIEW_FLAG_NOTOKEN    0x00000004
#define VIEW_FLAG_NOJSON     0x00000008
#define VIEW_FLAG_KINDS      0x00000010
#define VIEW_FLAG_OBJECTS   0x00000020
#define VIEW_FLAG_INDEXIDS   0x00000040
#define VIEW_FLAG_SEQ        0x00000080
#define VIEW_FLAG_INDEXES    0x00000100

const char* arrDbName[] = {
    "ALL", //DB8_TYPE_ALL,
    "kinds.db", //DB8_TYPE_KINDS,
    "objects.db", //DB8_TYPE_OBJECTS,
    "indexIds.db", //DB8_TYPE_INDEXIDS,
    "indexes.ldb", //DB8_TYPE_INDEXES,
    "seq.ldb", //DB8_TYPE_SEQ,
    "Catalog", //DB8_TYPE_CATALOG,
    "UsageDbName", //DB8_TYPE_DBNAME,
};

class db8objects_t
{
    public:
        db8objects_t();
        db8objects_t(const db8objects_t& other);
        LdbHeader header;
        unique_ptr<LdbObject> pObj;
        db8objects_t& operator=(db8objects_t& other);
};

db8objects_t::db8objects_t()
{
    pObj = unique_ptr<LdbObject>();
}

db8objects_t::db8objects_t(const db8objects_t& other)
{
    header = other.header;
    pObj = unique_ptr<LdbObject>();
    if(other.pObj.get() != NULL) {
        *pObj = *other.pObj;
    }
}

db8objects_t& db8objects_t::operator=(db8objects_t& other)
{
    header = other.header;
    pObj = unique_ptr<LdbObject>();
    if(other.pObj.get() != NULL) {
        *pObj = *other.pObj;
    }
    return *this;
}

typedef map<string,db8objects_t> map_objectdesc;

struct db8category_t
{
    unsigned char arrPrefix[3];
    map<string,string> m;
    DB8_TYPE type;
    map_objectdesc obj;
    bool isIndex;
    bool isHeader;
};
typedef map<string,db8category_t> map_category;

void print_char(unsigned char ch);
void print_string(string str);
void print_hexa(string str);

class DB8Info
{
    public:
        DB8Info() { }
        ~DB8Info() { }
        leveldb::DB* m_db;
        map_category m_info;
        db8tokens_map_t m_tokens_map;
        bool m_bNoTokens;
        bool m_bNoJson;

        bool setDB8Info();
        bool printDB8Info(DB8_TYPE type);
        bool dumpAll();
        bool setNoTokensFlag(bool flag) { m_bNoTokens = flag; return true; }
        bool setNoJsonFlag(bool flag) { m_bNoJson = flag; return true; }
        bool setJsonFlag(bool flag) { m_bNoTokens = flag; return true; }
    private:
        bool setInfoForCategory(unsigned char*, map<string,string>& m);
        bool setObjectsInfo(db8category_t& cat, map_objectdesc& target);
        bool printInfo(string title, db8category_t& cat);
        bool printTitle(string title, db8category_t& cat);
        bool printAll();
        bool printDepth(int depth);
        bool printAllJSON();
        bool printCatalogJSON(string title, db8category_t& cat, int depth);
        bool printObjects(db8category_t& cat);
        bool printObjectsJSON(db8category_t& cat, int depth);
        bool printObjectHeader(LdbHeader& header);
        bool setTokensMap(db8tokens_map_t& keymap);
};

static DB8_TYPE GetType(const char* name)
{
    if(strcmp(name, "Catalog") == 0) {
        return DB8_TYPE_CATALOG;
    }
    else if(strcmp(name, "UsageDbName") == 0) {
        return DB8_TYPE_DBNAME;
    }
    else if(strcmp(name, "indexIds.db") == 0) {
        return DB8_TYPE_INDEXIDS;
    }
    else if(strcmp(name, "indexes.ldb") == 0) {
        return DB8_TYPE_INDEXES;
    }
    else if(strcmp(name, "kinds.db") == 0) {
        return DB8_TYPE_KINDS;
    }
    else if(strcmp(name, "objects.db") == 0) {
        return DB8_TYPE_OBJECTS;
    }
    else if(strcmp(name, "seq.ldb") == 0) {
        return DB8_TYPE_SEQ;
    } else {
        return DB8_TYPE_ALL;
    }
}

bool DB8Info::setDB8Info()
{
    unsigned char arrPrefix[] = {'\0', '\0'};

    m_tokens_map.si.clear();
    m_tokens_map.imis.clear();
    m_bNoTokens = false;
    m_bNoJson = false;

    m_info[string("Catalog")] = db8category_t();
    m_info[string("Catalog")].arrPrefix[0] = arrPrefix[0];
    m_info[string("Catalog")].arrPrefix[1] = arrPrefix[1];
    m_info[string("Catalog")].type = DB8_TYPE_CATALOG;
    map<string,string>& m = m_info[string("Catalog")].m;

    if( setInfoForCategory(arrPrefix, m) == false ) { return false; }

    for(map<string,string>::iterator it = m.begin() ; it != m.end() ; it++) {
        m_info[it->first] = db8category_t();
        m_info[it->first].arrPrefix[0] = it->second[0];
        m_info[it->first].arrPrefix[1] = it->second[1];
        m_info[it->first].type = GetType(it->first.c_str());
        map<string,string>& m_cat = m_info[it->first].m;
        arrPrefix[0] = it->second[0];
        arrPrefix[1] = it->second[1];
        if( setInfoForCategory(arrPrefix, m_cat) == false ) { return false; }
        if( strcmp(it->first.c_str(), "objects.db") == 0) { m_info[it->first].isHeader = true; }
        else { m_info[it->first].isHeader = false; }
        if (strcmp(it->first.c_str(), "indexes.ldb") == 0) { m_info[it->first].isIndex = true; }
        else { m_info[it->first].isIndex = false; }
        if(setObjectsInfo(m_info[it->first], m_info[it->first].obj) == false) { return false; }
    }
    if(setTokensMap(m_tokens_map) == false) { return false; }
    return true;
}

bool DB8Info::setObjectsInfo(db8category_t& cat, map_objectdesc& target_map)
{
    unsigned char* pBuf;
    int size;

    bool isHeader = cat.isHeader;
    bool isIndexes = cat.isIndex;

    db8objects_t temp;
    temp.header.resetRead();
    temp.pObj.reset();
    target_map.clear();

    for(map_str::iterator it = cat.m.begin() ; it != cat.m.end() ; it++) {
        int pos = 0;
        target_map[it->first] = temp;
        db8objects_t& cur = target_map[it->first];

        if(isIndexes == false) {
            pBuf = (unsigned char*)it->second.data();
            size = it->second.length();
        } else {
            pBuf = (unsigned char*)it->first.data();
            size = it->first.length();
        }

        if(isHeader) {
            pos = cur.header.parseBuffer(pBuf, size);
            if(pos <= 0) {
                cerr << "Illegal position value" << endl;
                return false;
            }
        }
        unique_ptr<LdbObject> retObj = std::move(LdbParser::parseBuffer(pBuf + pos, size - pos, isIndexes));
        if(retObj.get() == NULL) return false;
        cur.pObj = std::move(retObj);
    }
    return true;
}

bool DB8Info::printDB8Info(DB8_TYPE type)
{
    switch(type) {
        case DB8_TYPE_KINDS:
        case DB8_TYPE_DBNAME:
        case DB8_TYPE_OBJECTS:
        case DB8_TYPE_INDEXIDS:
        case DB8_TYPE_INDEXES:
        case DB8_TYPE_SEQ:
            {
                if(m_bNoJson) {
                    db8category_t& cat = m_info[string(arrDbName[type])];
                    if(printObjects(cat) == false) { return false; }
                } else {
                    db8category_t& cat = m_info[string(arrDbName[type])];
                    if(printObjectsJSON(cat, 0) == false) { return false; }
                }
            }
            break;
        case DB8_TYPE_ALL:
            if(m_bNoJson) {
                if(printAll() == false) { return false; }
            } else {
                if(printAllJSON() == false) { return false; }
            }
            break;
        default:
            break;
    }
    return true;
}

bool DB8Info::dumpAll()
{
    for(map_category::iterator it = m_info.begin() ; it != m_info.end() ; it++) {
        if(printInfo(it->first, it->second) == false) { return false; }
    }
    return true;
}

bool DB8Info::setInfoForCategory(unsigned char* arrPrefix, map<string,string>& m)
{
    leveldb::Slice prefix;
    leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());

    m.clear();

    prefix = leveldb::Slice((const char*)arrPrefix);

    it->Seek(prefix);
    while (it->Valid()) {
        string key1 = it->key().ToString();
        string value = it->value().ToString();

        if(key1.length() < 2 || (key1[0] != arrPrefix[0] || key1[1] != arrPrefix[1])) {
            break;
        } else if(key1.length() > 2) {
            if(key1[2] == 0x04) {
                m.insert(pair<string, string>(key1.substr(3), value));
            } else {
                m.insert(pair<string, string>(key1.substr(2), value));
            }
        }
        it->Next();
    }
    delete it;
    return true;
}

bool DB8Info::printInfo(string title, db8category_t& cat)
{
    cout << "------- ";
    print_string(title);
    cout << ": " << cat.m.size();
    cout << " (";
    print_char(cat.arrPrefix[0]);
    print_char(cat.arrPrefix[1]);
    cout << ")";
    cout << " -------" << endl;

    for(map<string,string>::iterator it = cat.m.begin() ; it != cat.m.end() ; it++) {
        print_string(it->first);
        cout << ": ";
        print_string(it->second);
        cout << endl;
    }
    return true;
}

bool DB8Info::printTitle(string title, db8category_t& cat)
{
    cout << "------- ";
    print_string(title);
    cout << ": " << cat.m.size();
    cout << " (";
    print_char(cat.arrPrefix[0]);
    print_char(cat.arrPrefix[1]);
    cout << ")";
    cout << " -------" << endl;

    return true;
}

bool DB8Info::printAll()
{
    for(map_category::iterator it = m_info.begin() ; it != m_info.end() ; it++) {
        switch(it->second.type) {
            case DB8_TYPE_CATALOG:
                if(printInfo(it->first, it->second) == false) { return false; }
                break;
            case DB8_TYPE_DBNAME:
            case DB8_TYPE_INDEXIDS:
            case DB8_TYPE_INDEXES:
            case DB8_TYPE_KINDS:
            case DB8_TYPE_OBJECTS:
            case DB8_TYPE_SEQ:
                {
                    db8category_t& cat = m_info[string(arrDbName[it->second.type])];
                    if(printTitle(it->first, it->second) == false) { return false; }
                    if(printObjects(cat) == false) { return false; }
                }
                break;
            default:
                cerr << "Illegal type value" << endl;
                return false;
                break;
        }
    }
    return true;
}

bool DB8Info::printCatalogJSON(string title, db8category_t& cat, int depth)
{
    for(map<string,string>::iterator it = cat.m.begin() ; it != cat.m.end() ; ) {
        printDepth(depth);
        cout << "\"";
        print_string(it->first);
        cout << "\":\"";
        print_hexa(it->second);
        cout << "\"";
        it++;
        if(it != cat.m.end()) {
            cout << "," << endl;
        } else {
            cout << endl;
        }
    }
    return true;
}

bool DB8Info::printDepth(int depth)
{
    for(int i = 0 ; i < depth ; i++) {
        cout << ' ';
    }
    return true;
}

bool DB8Info::printAllJSON()
{
    int depth = 0;
    cout << "{" << endl;
    depth+=2;
    for(map_category::iterator it = m_info.begin() ; it != m_info.end() ; ) {
        printDepth(depth);
        cout << "\"" << it->first << "\":{" << endl;
        depth+= 2;
        switch(it->second.type) {
            case DB8_TYPE_CATALOG:
                if(printCatalogJSON(it->first, it->second, depth) == false) { return false; }
                break;
            case DB8_TYPE_DBNAME:
            case DB8_TYPE_INDEXIDS:
            case DB8_TYPE_INDEXES:
            case DB8_TYPE_KINDS:
            case DB8_TYPE_OBJECTS:
            case DB8_TYPE_SEQ:
                {
                    db8category_t& cat = m_info[string(arrDbName[it->second.type])];
                    if(printObjectsJSON(cat, depth) == false) { return false; }
                }
                break;
            default:
                cerr << "Illegal type value" << endl;
                return false;
                break;
        }
        depth-=2;
        it++;
        printDepth(depth);
        if(it != m_info.end()) {
            cout << "}," << endl;
        } else {
            cout << "}" << endl;
        }
    }
    depth-=2;
    cout << "}" << endl;
    return true;
}

bool DB8Info::printObjects(db8category_t& cat)
{
    map_objectdesc& target_map = cat.obj;
    bool isIndexes = cat.isIndex;
    DB8_TYPE type = cat.type;

    db8tokens_map_t nullkeys;
    for(map_objectdesc::iterator it = target_map.begin() ; it != target_map.end() ; it++) {
        db8objects_t& desc = it->second;
        if(desc.pObj) {
            if(isIndexes == false) {
                print_string(it->first);
                cout << " - ";

                printObjectHeader(desc.header);
                i64 kind_id = 0;
                if(m_bNoTokens == false && type == DB8_TYPE_OBJECTS && desc.header.getRead()) {
                    kind_id = desc.header.getKindId();
                }

                string out;
                if(LdbJsonViewer::view(desc.pObj, out, m_tokens_map.imis[kind_id]) == false) {
                    cerr << "viewer function failed" << endl;
                    return false;
                }
                print_string(out);
                cout << endl;
            } else {
                string out;
                if(LdbJsonViewer::view(desc.pObj, out, m_tokens_map.imis[0]) == false) {
                    cerr << "viewer function failed" << endl;
                    return false;
                }
                print_string(out);
                cout << endl;
            }
        }
    }
    return true;
}

static LdbType getFirstObjType(unique_ptr<LdbObject>& pFirstObj)
{
    return pFirstObj->data().front().value().getType();
}

static unique_ptr<LdbObject>& getFirstObj(unique_ptr<LdbObject>& pFirstObj)
{
    return pFirstObj->data().front().value().getObject();
}

static unique_ptr<LdbArray>& getFirstArray(unique_ptr<LdbObject>& pFirstObj)
{
    return pFirstObj->data().front().value().getArray();
}

bool DB8Info::printObjectsJSON(db8category_t& cat, int depth)
{
    map_objectdesc& target_map = cat.obj;
    bool isIndexes = cat.isIndex;
    DB8_TYPE type = cat.type;

    db8tokens_map_t nullkeys;
    for(map_objectdesc::iterator it = target_map.begin() ; it != target_map.end() ; ) {
        db8objects_t& desc = it->second;
        if(desc.pObj) {
            i64 kind_id = 0;
            printDepth(depth);
            if(isIndexes == false) {
                cout << "\"";
                print_string(it->first);
                cout << "\":";

                if(desc.header.getRead()) {
                    cout << "{\"Header\":";
                    printObjectHeader(desc.header);
                    cout << ",";
                    if(m_bNoTokens == false && type == DB8_TYPE_OBJECTS && desc.header.getRead()) {
                        kind_id = desc.header.getKindId();
                    }
                }
            }

            if(desc.header.getRead()) {
                cout << "\"Object\":";
            }

            string out;
            LdbType type = getFirstObjType(desc.pObj);
            if(type == TypeObject) {
                if(LdbJsonViewer::view(getFirstObj(desc.pObj), out, m_tokens_map.imis[kind_id]) == false) {
                    cerr << "viewer function failed" << endl;
                    return false;
                }
                cout << "{";
                print_string(out);
                cout << "}";
            } else {
                if(LdbJsonViewer::arrayView(getFirstArray(desc.pObj), out, m_tokens_map.imis[kind_id]) == false) {
                    cerr << "viewer function failed" << endl;
                    return false;
                }
                if(isIndexes) {
                    cout << "\"Key\":[";
                    print_string(out);
                    cout << "]";
                } else {
                    print_string(out);
                }
            }
            if(desc.header.getRead()) {
                cout << "}";
            }
            it++;
            if(it != target_map.end()) {
                cout << "," << endl;
            } else {
                cout << endl;
            }
        }
    }
    return true;
}

bool DB8Info::printObjectHeader(LdbHeader& header)
{
    if(!header.getRead()) return false;

    string& kname = header.getKindIdName();

    if(kname.empty()) {
        cout << "[" << (int)header.getVersion() << ","
            << header.getKindId() << ","
            << header.getRev()
            << (header.getDel() ? ",true":",false") << "]";
    } else {
        cout << "[" << (int)header.getVersion() << ","
            << kname << ","
            << header.getRev()
            << (header.getDel() ? ",true":",false") << "]";
    }

    return true;
}

static LdbData noneData;

static LdbData& getKindTokens(unique_ptr<LdbObject>& pObj)
{
    LdbObject* pTarget = NULL;
    if(pObj == NULL) { return noneData; }

    for(list<LdbData>::iterator it = pObj->data().begin() ; it != pObj->data().end() ; it++) {
        if(it->value().getType() == TypeObject) {
            LdbData& dat = getKindTokens(it->value().getObject());
            if(dat.prop().getPropertyName() == string("kindTokens")) {
                return dat;
            }
        }
        if(it->prop().getPropertyName() == string("kindTokens")) {
            return *it;
        }
    }
    return noneData;
}

static list<LdbData>& getTokenList(unique_ptr<LdbObject>& pObj)
{
    LdbData& firstdata = pObj->data().front();
    unique_ptr<LdbObject>& pTokenObj1 = firstdata.value().getObject();
    unique_ptr<LdbObject>& pTokenObj2 = pTokenObj1->data().front().value().getObject();
    return pTokenObj2->data();
}

static bool setTokens(list<LdbData>& datalist, map_is& is)
{
    for(list<LdbData>::iterator it = datalist.begin() ; it != datalist.end() ; it++) {
        string tokens;
        int tokeni;

        tokens = it->prop().getPropertyName();
        tokeni = it->value().getUInt();
        is[tokeni] = tokens;
    }
    return true;
}

bool DB8Info::setTokensMap(db8tokens_map_t& keymap)
{
    map_si& si = keymap.si;
    map_imis& imis = keymap.imis;
    map_is empty_is;

    map_objectdesc& indexids = m_info[arrDbName[DB8_TYPE_INDEXIDS]].obj;
    map_objectdesc& kinds = m_info[arrDbName[DB8_TYPE_KINDS]].obj;

    //get kindTokens from indexids
    for(map_objectdesc::iterator it = indexids.begin() ; it != indexids.end() ; it++) {
        LdbData& data = getKindTokens(it->second.pObj);
        if(data.prop().getPropertyName() == string("kindTokens")) {
            string tokens;
            int tokeni;

            tokens = data.value().getObject()->data().front().prop().getPropertyName();
            tokeni = data.value().getObject()->data().front().value().getUInt();
            si[tokens] = tokeni;
            imis[tokeni] = empty_is;
        }
    }
    //set tokens from kinds
    for(map_objectdesc::iterator it = kinds.begin() ; it != kinds.end() ; it++) {
        int kind_id = si[string(it->first.c_str())];

        list<LdbData>& datalist = getTokenList(it->second.pObj);
        if(setTokens(datalist, imis[kind_id]) == false) {
            return false;
        }
    }

    return true;
}

class LeveldbViewer
{
    private:
        char m_zPath[256];
        leveldb::DB* m_db;
        leveldb::Options m_opt;
        bool m_isOpen;

        unique_ptr<DB8Info> m_pdb8info;

    public:
        LeveldbViewer();
        LeveldbViewer(char* zPath);
        ~LeveldbViewer();

        bool Open();
        bool Open(char* zPath);
        bool Close();

        bool DumpAll();
        bool DumpDB8();
        bool SetDB8Info();
        bool PrintDB8Info(DB8_TYPE type);
        bool SetNoTokensFlag(bool flag) { return m_pdb8info->setNoTokensFlag(flag); }
        bool SetNoJsonFlag(bool flag) { return m_pdb8info->setNoJsonFlag(flag); }

    protected:
        bool dumpDB8_catalog();
};

LeveldbViewer::LeveldbViewer()
{
    m_pdb8info = unique_ptr<DB8Info>(new DB8Info);
    m_zPath[0] = '\0';
    m_db = NULL;
    m_isOpen = false;
    m_opt.create_if_missing = true;
    cerr << "LeveldbViewer init" << endl;
}

LeveldbViewer::LeveldbViewer(char* zPath)
{
    LeveldbViewer();
    strcpy(m_zPath, zPath);
}

LeveldbViewer::~LeveldbViewer()
{
    Close();
}

bool LeveldbViewer::Open()
{
    leveldb::Status status;
    if(m_isOpen == true || m_zPath[0] == '\0') {
        cerr << "alread open or path is null" << endl;
        return false;
    }
    status = leveldb::DB::Open(m_opt, m_zPath, &m_db);
    if(!status.ok()) {
        cerr << "leveldb Open failed" << endl;
        std::cerr << status.ToString() << endl;
        return false;
    }
    m_isOpen = true;
    m_pdb8info->m_db = m_db;
    return true;
}

bool LeveldbViewer::Open(char* zPath)
{
    strcpy(m_zPath, zPath);
    return Open();
}

bool LeveldbViewer::Close()
{
    if(m_isOpen) {
        delete m_db;
        m_db = NULL;
        m_isOpen = false;
    }
    return true;
}

bool LeveldbViewer::DumpAll()
{
    return m_pdb8info->dumpAll();
}

static char hexarr[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

void print_char(unsigned char ch)
{
    if(ch == ' ' || (isprint(ch) && !isspace(ch))) {
        cout << ch;
    } else {
        cout << "\\x" << hexarr[ch>>4] << hexarr[ch&0x0f];
    }
}

void print_string(string str)
{
    int len = (int)str.length();
    for( int i  = 0 ; i < len && str[i] != 0 ; i++) {
        print_char(str[i]);
    }
}

void print_hexa(string str)
{
    int len = (int)str.length();
    for( int i  = 0 ; i < len ; i++) {
        cout << "\\x" << hexarr[str[i]>>4] << hexarr[str[i]&0x0f];
    }
}

bool LeveldbViewer::DumpDB8()
{
    if(dumpDB8_catalog() == false) { return false; }
    return true;
}

bool LeveldbViewer::dumpDB8_catalog()
{
    return true;
}

bool LeveldbViewer::SetDB8Info()
{
    return m_pdb8info->setDB8Info();
}

bool LeveldbViewer::PrintDB8Info(DB8_TYPE type)
{
    return m_pdb8info->printDB8Info(type);
}

int main(int argc, char* argv[])
{
    LeveldbViewer viewer;
    bool ret;
    LdbObject obj;
    VIEW_FLAG flag = VIEW_FLAG_NONE;

    if(argc < 2) {
        cout << "Usage: ldbviewer \"db path\" [options]" << endl;
        cout << "Options:" << endl;
        cout << "         all:       print all of contents (default)" << endl;
        cout << "         raw:       print all of contents in raw format" << endl;
        cout << "         no_json:   use custom format" << endl;
        cout << "         no_tokens: don't use token matching for objects" << endl;
        cout << "         [kinds|objects|indexes|indexids|seq]: print selected category" << endl;
        return 1;
    }
    ret = viewer.Open(argv[1]);
    if(ret == false) goto main_end;
    if(viewer.SetDB8Info() == false) {
        cerr << "DB parsing failed" << endl;
        viewer.Close();
        return 1;
    }
    if(argc <= 2) {
        flag |= VIEW_FLAG_ALL;
    }
    for(int f = 2 ; f < argc ; f++) {
        if(strcmp(argv[f], "all") == 0) {
            flag |= VIEW_FLAG_ALL;
        } else if(strcmp(argv[f], "raw") == 0) {
            flag |= VIEW_FLAG_RAW;
        } else if(strcmp(argv[f], "no_json") == 0) {
            flag |= VIEW_FLAG_NOJSON;
        } else if(strcmp(argv[f], "no_tokens") == 0) {
            flag |= VIEW_FLAG_NOTOKEN;
        } else if(strcmp(argv[f], "kinds") == 0) {
            flag |= VIEW_FLAG_KINDS;
        } else if(strcmp(argv[f], "objects") == 0) {
            flag |= VIEW_FLAG_OBJECTS;
        } else if(strcmp(argv[f], "indexids") == 0) {
            flag |= VIEW_FLAG_INDEXIDS;
        } else if(strcmp(argv[f], "seq") == 0) {
            flag |= VIEW_FLAG_SEQ;
        } else if(strcmp(argv[f], "indexes") == 0) {
            flag |= VIEW_FLAG_INDEXES;
        }
    }
    if(flag & VIEW_FLAG_NOJSON) {
        viewer.SetNoJsonFlag(true);
    }
    if(flag & VIEW_FLAG_NOTOKEN) {
        viewer.SetNoTokensFlag(true);
    }
    if(flag & VIEW_FLAG_RAW) {
        viewer.DumpAll();
    } else if(flag & VIEW_FLAG_ALL) {
        viewer.PrintDB8Info(DB8_TYPE_ALL);
    } else {
        if(flag & VIEW_FLAG_KINDS) {
            viewer.PrintDB8Info(DB8_TYPE_KINDS);
        }
        if(flag & VIEW_FLAG_OBJECTS) {
            viewer.PrintDB8Info(DB8_TYPE_OBJECTS);
        }
        if(flag & VIEW_FLAG_INDEXIDS) {
            viewer.PrintDB8Info(DB8_TYPE_INDEXIDS);
        }
        if(flag & VIEW_FLAG_SEQ) {
            viewer.PrintDB8Info(DB8_TYPE_SEQ);
        }
        if(flag & VIEW_FLAG_INDEXES) {
            viewer.PrintDB8Info(DB8_TYPE_INDEXES);
        }
    }
    ret = viewer.Close();
    if(ret == false) goto main_end;

    return 0;
main_end:
    cerr << "WTF!!!" << endl;
    return 1;
}
