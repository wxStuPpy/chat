#pragma once
#include"const.h"

struct SectionInfo{
    SectionInfo(){}
    ~SectionInfo(){_section_datas.clear();}
    SectionInfo(const SectionInfo& other){
        _section_datas = other._section_datas;
    }
    SectionInfo& operator=(const SectionInfo& other){
        if(this != &other)       
            _section_datas = other._section_datas; 
        return *this;
    }
    std::map<std::string, std::string> _section_datas;
    std::string operator[](const std::string& key){
        if(_section_datas.find(key) != _section_datas.end())
            return _section_datas[key];
        else
          return "";
    }
    std::string getValue(const std::string & key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}
};

class ConfigMgr {
public:
   ~ConfigMgr(){_sections.clear();}
   SectionInfo operator[](const std::string& key){
        if(_sections.find(key) == _sections.end()){
          return SectionInfo();
        }
        return _sections[key];
    }
    ConfigMgr& operator=(const ConfigMgr& other)=delete;
    ConfigMgr(const ConfigMgr& other)=delete;

    static ConfigMgr&getInstance(){
        static ConfigMgr instance;
        return instance;
    }
    std::string getValue(const std::string& section, const std::string & key);
private:
   ConfigMgr();
  std::map<std::string, SectionInfo> _sections;
};