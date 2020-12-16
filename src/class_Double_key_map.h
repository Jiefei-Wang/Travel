#ifndef HEADER_DOUBLE_KEY_MAP
#define HEADER_DOUBLE_KEY_MAP
#include <map>


template <class KEY1, class KEY2, class VALUE> 
class Double_key_map { 
private: 
    std::map<const KEY1, VALUE> coreMap;
    std::map<const KEY2, const KEY1> key2_key1_Map;
    std::map<const KEY1, const KEY2> key1_key2_Map;
    void erase(const KEY1 key1, const KEY2 key2){
        coreMap.erase(key1);
        key2_key1_Map.erase(key2);
        key1_key2_Map.erase(key1);
    }
public: 
    void insert(const KEY1 key1,const KEY2 key2, VALUE value){
        if(has_key1(key1)){
            erase_value_by_key1(key1);
        }
        if(has_key2(key2)){
            erase_value_by_key2(key2);
        }
        coreMap.insert(std::pair<const KEY1, VALUE>(key1, value));
        key2_key1_Map.insert(std::pair<const KEY2, const KEY1>(key2, key1));
        key1_key2_Map.insert(std::pair<const KEY1, const KEY2>(key1, key2));
    }
    VALUE& get_value_by_key1(const KEY1 key1){
        VALUE& value = coreMap.at(key1);
        return value;
    }
    VALUE& get_value_by_key2(const KEY2 key2){
        return get_value_by_key1(get_key1(key2));
    }
    bool has_key1(const KEY1 key1){
        bool result = key1_key2_Map.find(key1)!=key1_key2_Map.end();
        return result;
    }
    bool has_key2(const KEY2 key2){
        bool result = key2_key1_Map.find(key2)!=key2_key1_Map.end();
        return result;
    }
    bool erase_value_by_key1(const KEY1 key1){
        if(!has_key1(key1)){
            return false;
        }
        erase(key1, get_key2(key1));
        return true;
    }
    bool erase_value_by_key2(const KEY2 key2){
        if(!has_key2(key2)){
            return false;
        }
        erase(get_key1(key2), key2);
        return true;
    }
    const KEY2& get_key2(const KEY1 key1){
        const KEY2& key2 = key1_key2_Map.at(key1);
        return key2;
    }
    const KEY1& get_key1(const KEY2 key2){
        const KEY1& key1 = key2_key1_Map.at(key2);
        return key1;
    }
    typename std::map<const KEY1,const KEY2>::iterator begin_key(){
        return key1_key2_Map.begin();
    }
    typename std::map<const KEY1,const KEY2>::iterator end_key(){
        return key1_key2_Map.end();
    }
    size_t size(){
        return coreMap.size();
    }
}; 


#endif