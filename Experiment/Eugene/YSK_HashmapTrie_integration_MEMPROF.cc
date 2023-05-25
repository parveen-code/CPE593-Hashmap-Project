/*
Authors: Eugene Kozlakov, Parveen Sabah, and Yihang Yuan
Sources and References:
*/

//#define DATASETPATH "C:\\Users\\eugko\\YSKCompressedHashmapDict\\dataset\\"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <string>
#include <windows.h>
#include <psapi.h>


using namespace std;

class trieHash {

  private:
    class Node {
      public:
        bool isPrefix = false;; // denotes whether word set is valid prefix. If true, jump to hashtable via pointer.
        bool isWord = false; //in case we get a word that is completely unique -- no shared prefixes. will simply be mapped to trie.
        unordered_map<unsigned long, string>* suffixes = nullptr; // seperate unordered map that maps one 
        Node* next[26];
        Node() : isPrefix(false) { // constructor
          //write loops
          for(int i=0; i<26; i++){ //ensuring first 26 elements in root are null
            next[i] = nullptr;
          }
        }
        /*
        ~Node(){
          suffixes->clear();
          //delete suffixes;
          //delete [] next;
        }
        */
    };
    
  public:
    Node root; //start of trie //how to initialize...?
    uint32_t maxHashSize; // will indicate maxsize of values in key-value pairs.

  public:
    trieHash() : root(){}
    ~trieHash(){ //need to come up with a proper destructor.
      delAll(&root); // destructor will just call existing function.
    }

    unsigned long hashString(unsigned char *str) { // hashing function for retrieving words once pushed into triehash
      unsigned long hash = 5381;
      int c; // when assgin char to int, it convert by UTF-8

      while ((c = *str++)) {
          hash = ((hash << 5) + hash) + c; // equivalent to hash = hash * 33 + c
      }

      return hash;
    }

    void addPref(string pref){
      char c;
      Node* p = &root; // starting from the root.
      for (int i = 0; i<pref.length(); i++){ // iterating through every letter, from zero to length of word.
        c = pref[i] - 'a';  //setting char so that the 'c' variable gets assigned a value anywhere from 0-25: the 26 letters of the alphabet.
        if(p->next[c] == nullptr){
          //cout << "adding new node" << endl;
          p->next[c] = new Node(); // if the character in the next trie is null, set it to the following letter. //why error?
        }
        p = p->next[c];
      }
      p->isPrefix = true;
    }

    void addWord(string word){//added function to account for dictionaries with 1 key-value pair, or individual user words.
      char c;
      Node* p = &root; // starting from the root.
      unordered_map<unsigned long, string>*  lastAvailableHash = nullptr;
      for (int i = 0; i<word.length(); i++){ // iterating through every letter, from zero to length of word.
        c = word[i] - 'a';  //setting char so that the 'c' variable gets assigned a value anywhere from 0-25: the 26 letters of the alphabet.
        if(p->isPrefix == true && (p->suffixes->size() < maxHashSize)){ //i think the < part may prove to be a logical issue-- not yet tested.
          lastAvailableHash = p->suffixes;
        }
        if(p->next[c] == nullptr){
          //cout << "adding new node" << endl;
          p->next[c] = new Node(); // if the character in the next trie is null, set it to the following letter. //why error?
        }
        p = p->next[c];
      }
      if(lastAvailableHash != nullptr){ // if there is a prefix for this word with an avilable hashtable, map word to hashtable.
        const char* suffixChar = word.c_str();
        unsigned long hashValue = hashString((unsigned char *)suffixChar);
        lastAvailableHash->emplace(hashValue, word);
        return;
      }

      p->isWord = true;
      return;
    }

    void delAll (Node* p){ //delete entire library when program closes.
      for(int i=0; i<26; i++){
        if(p->next[i] != nullptr){
          delAll(p->next[i]);
        }
      }
      //delete p->suffixes;
      delete p;
    }

    //checks if prefix exists.
    bool checkPrefix(string word){
      Node* p = &root;
      for(int i = 0; i < word.length(); i++){
        int c = word[i] - 'a';
        if(p->next[c] != nullptr){
            p = p->next[c];
        } else {
          cout << "[" << word << "] has *NOT* been found in the dictionary." << endl;
          return false;
        }
      }

      if (p->isPrefix == true){
        cout << "[" << word << "] has been found in the dictionary."<< endl;
        return true;
      } else {
        cout << "[" << word << "] has *NOT* been found in the dictionary."<< endl;
        return false;
      }
    }

    //makes hash table at every leaf node where a valid prefix is present.
    void makeHashTable(string prefixString, const vector<string>& breakerSuffixList){ //will make one hashtable. will be called multiple times.
      Node* p = &root;
      
      //locate leaf node
      for(int i = 0; i < prefixString.length(); i++){
        int c = prefixString[i] - 'a';
        if(p->next[c] != nullptr){
            p = p->next[c];
        } else {
          cout << "prefix [" << prefixString << "] has *NOT* been found in the dictionary." << endl;
          return;
        }
      }

      //if prefix is genuine leaf node, make a new hashtable (unordered map) at leaf.
      if (p->isPrefix == true){
        p->suffixes = new unordered_map<unsigned long, string>;
      } else {
        cout << "prefix [" << prefixString << "] has *NOT* been found in the dictionary. Aborting Hashtable."<< endl;
        return;
      }

      unordered_map<unsigned long, string>* hashTable = p->suffixes; //making hashtable ptr variable to avoid making pointer to pointer calls

      // now that we have made a new hashtable at the leaf node a of a genuine prefix, we will assign the hash
      // value of each suffix  
      const char *suffixChar;
      unsigned long hashValue;
      for(auto const& item:breakerSuffixList){
        suffixChar = item.c_str(); //locate every single word of string datatype, convert to char for concatenation
        hashValue = hashString((unsigned char *)suffixChar);
        hashTable->emplace(hashValue, string(suffixChar));

      }
    }

    void findWord(string word){
      Node* p = & root;
      unsigned long hashValue = hashString((unsigned char *)word.c_str());
      string prefix = "";
      
      for(int i = 0; i < word.length(); i++){
        int c = word[i] - 'a';
        if(p->next[c] != nullptr){
            p = p->next[c];
            auto hashTable = p->suffixes;
          if(p->isPrefix == true){
            auto pair = hashTable->find(hashValue);
            if (pair == hashTable->end()){
              continue;
            } else {
              cout << " found: [" << pair->second << "]" << endl;
              break;
            }
          }
        } else if (p->isWord == true){
          cout << "[" << word << "]" << "has been found in the dictionary." << endl;
        } else {
          cout << "[" << word << "] has *NOT* been found in the dictionary." << endl;
          return;
        }
          
      }
    }

};


//IMPORTANT NOTICE:
//  Yihang Yuan wrote this function initially in Python, and translated it to C++
//  with the help of ChatGPT. With a bit of tweaking from Eugene Kozlakov, we 
//  implemented the "breaker()" function here.
void breaker (const vector<string>& intake, unordered_map<string, vector<string>>& dictionary, uint32_t maxHashSize){ //function that performs everything in main
  //unordered_map<string, vector<string>> dictionary;
  unordered_map<int, char> letter_to_num;

  dictionary[""] = intake;

  while (true) {
      bool all_less_than_max = true;
      for (auto const& pair : dictionary) {
          if (pair.second.size() > maxHashSize) {
              all_less_than_max = false;
              break;
          }
      }
      if (all_less_than_max) {
          break;
      }

      vector<std::string> keys_to_delete;
      vector<std::string> keys_to_add_back;
      unordered_map<string, vector<string>> new_entries;

      vector<string> keys;
      for (auto const& pair : dictionary) {
          keys.push_back(pair.first);
      }

      for (const auto &prefix : keys) {
          vector<string> words_list = dictionary[prefix];
          if (words_list.size() > maxHashSize) {
              int len_prefix = prefix.length();
              for (int i = 0; i < 26; ++i) { //26 for 26 letters in alphabet
                  string new_key = prefix + letter_to_num[i];
                  new_entries[new_key] = {};
              }
              for (const auto &word : words_list) {
                  if (word.length() == len_prefix) {
                      keys_to_add_back.push_back(prefix);
                      continue;
                  }
                  new_entries[word.substr(0, len_prefix+1)].push_back(word);
              }
              for (int i = 0; i < 26; ++i) {
                  string key_to_check = prefix + letter_to_num[i];
                  if (new_entries[key_to_check].empty()) {
                      new_entries.erase(key_to_check);
                  }
              }
              keys_to_delete.push_back(prefix);
          }
      }

      // Delete old keys and merge new entries into dictionary
      for (const auto &key : keys_to_delete) {
          dictionary.erase(key);
      }

      for (const auto &key : keys_to_add_back) {
          dictionary[key] = {key};
      }

      // Merge new_entries into dictionary
      for (const auto &pair : new_entries) {
          dictionary[pair.first] = pair.second;
      }
  }

  //return dictionary;
}

int main(){
 
  const std::string DATAPATH{ "C:\\Users\\eugko\\YSKCompressedHashmapDict\\dataset\\" };
  unordered_map<string, vector<string>> dictionary;
  vector<string> intake;
  trieHash finalDict;
  std::fstream f(DATAPATH + "dict.txt");
  std::string word, prefix;
  std::string s;

  //MEMORY PROFILING
  PROCESS_MEMORY_COUNTERS_EX pmc;
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  
  
  //first we break down the dictionary into a hashtable (unorderded map) of prefixes (keys)
  // and a list of the subsequent suffixes (values.) 
  
  while (f){
    getline(f, word);
    intake.push_back(word);
  }

  //inputs defined:
  //breaker([intake list of words to distribute], [memaddress of wordlist broken into key-valuelist pairs], [maximum size of hashtable])
  finalDict.maxHashSize = 64; //number by design is meant to be modular for user needs and benchmarking.
  breaker(intake, dictionary, finalDict.maxHashSize); // breaker will break down the intake into giant hash table with prefixes as keys and suffix list as values.
  
  //now we have the defined "dictionary" hash table.
  //we will now map all of the keys to the trie.

  
  for(const auto& pair:dictionary){
    if(pair.second.size() == 1){
      finalDict.addWord(pair.second.front());
    }
    finalDict.addPref(pair.first);
    finalDict.makeHashTable(pair.first, pair.second);
  }

  prefix = "parasy";
  std::cout << "Checking for prefix [" << prefix << "] in dictionary." << std::endl;
  finalDict.checkPrefix(prefix);

  word = "parasynapsis";
  std::cout << "Checking for word [" << word << "] in dictionary." << std::endl;
  finalDict.findWord(word);
  
  word = "raaah";
  std::cout << "Inputting word [" << word <<" in dictionary" << std::endl;
  finalDict.addWord(word);
  
  std::cout << "Searching for ["<< word << "] in dictionary: " << std::endl;
  finalDict.findWord("raaah");

  word = "parasyxx";
  std::cout << "adding word ["<< word << "] in dictionary" << std::endl; //'parasyxx' doesn't exist. it's meant to test if the word will still hash.
  finalDict.addWord(word);

  std::cout << "Searching for [" << word << "] in dictionary: " << std::endl;
  finalDict.findWord(word);

  std::cout << "Goodbye." << std::endl;

  
  //GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  //SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
  //std::cout << "PhysMemUsedByMe: " << physMemUsedByMe << endl;

  return 0;
}