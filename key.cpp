#include "btree.hpp"
#include <cassert>

using namespace std;

int keyLength(KeyType *keytype){
  int total_size = 0;
  for(int i = 0; i < keytype->numAttrs; i++){
    total_size += (keytype->attrLen)[i];
  }
  return total_size;
}

int setIntAttrval(byte *key, KeyType * keytype, int attrnum, int val){
  assert(attrnum < keytype->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  int *newval = (int *)(key + seekSize);
  *(newval) = val;
  return 0;
}

int getIntAttrVal(byte *key, KeyType * keytype, int attrnum, int& retval){
  assert(attrnum < keytype->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  retval = *(int *)(key + seekSize);
  return 0;
}

int setStringAttrVal(byte *key, KeyType * keytype, int attrnum, char *val){
  assert(attrnum < keytype->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  strncpy(key + seekSize, val, keyLength(keytype));
  return 0;
}

int getStringAttrVal(byte *key, KeyType * keytype, int attrnum, char *retval){
  assert(attrnum < keytype->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  strncpy(retval,key + seekSize, keyLength(keytype));
  return 0;
}


int compareKeys(byte* key1, byte* key2, KeyType* keytype){


  //test
  
  if(DEBUG){
    cout << "comparekeys: " << *(int *)key1 << ","<< *(int *)key2 <<","<< (char *)(key1 + 4) <<","<<(char *)(key2 + 4) << ","<<*(int *)(key1 + 12)\
    << "," << *(int *)(key2 + 12) << endl; 
  }
  int i = 0;
  int offset1 = 0, offset2 = 0;
  while(i < keytype -> numAttrs){
    if (keytype -> attrTypes[i] == intType){
      int val1, val2;
      val1 = *(int *)(key1 + offset1);
      val2 = *(int *)(key2 + offset2);
      if (val1 < val2)
      {
	return -1;
      }
      else if (val1 > val2)
      {
	return 1;
      }
      else
      {
	offset1 += INTSIZE;
	offset2 +=  INTSIZE;
	i++;
	continue;
      }
    }
    else if (keytype -> attrTypes[i] == stringType){
      int cmp = strcmp((char *)(key1 + offset1), (char *)(key2 + offset2));
      if(DEBUG){
	cout << "string comparison " << (char *)(key1 + offset1) << " " << (char *)(key2 + offset2) << " " << cmp << endl;

      }
      if (cmp < 0)
      {
	return -1;
      }
      else if (cmp > 0)
      {
	return 1;
      }
      else{
	offset1 += strlen(key1 + offset1);
	offset2 += strlen(key2 + offset2);
	i++;
	continue;
      }
    }
  }
  return 0;
}
