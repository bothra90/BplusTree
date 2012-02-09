#include<btree.hpp>

using namespace std;

int keyLength(KeyType *keytype){
  int total_size = 0;
  for(int i = 0; i < keytype->numAttrs; i++){
    total_size += (keytype->attrLen)[i];
  }
  return total_size;
}

int setIntAttrval(byte *key, KeyType * keytype, int attrnum, int val){
  assert(attrnum < keytime->numAttrs);
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
  assert(attrnum < keytime->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  retval = *(int *)(key + seekSize);
  return 0;
}

int setStringAttrVal(byte *key, KeyType * keytype, int attrnum, char *val){
  assert(attrnum < keytime->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  strncpy(key + seekSize, val);
  return 0;
}

int getStringAttrVal(byte *key, KeyType * keytype, int attrnum, char *retval){
  assert(attrnum < keytime->numAttrs);
  int seekSize = 0;
  // assuming 0 indexed numbering of attributes
  for (int i = 0; i < attrnum ; i++) {
    seekSize += (keytype->attrLen)[i];
  }
  strncpy(retval,key + seekSize);
  return 0;
}


int compareKeys(byte* key1, byte* key2, KeyType* keytype){
  int i = 0;
  int offset1 = 0, offset2 = 0;
  while(i < keytype -> numAttrs){
    if (keytype -> attrType[i] == intType){
      int val1, val2;
      val1 = *(int *)(key1 + offset1);
      val2 = *(int *)(key2 + offset2);
      if (val1 < val2)
	return -1;
      else if (val1 > val2)
	return 1;
      else
      {
	offset1 += INTSIZE;
	offset2 +=  INTSIZE;
	continue;
      }
    }
    else if (keyType -> attrType[i] == stringType){
      int cmp = strcmp(key1 + offset1, key2 + offset2);
      if (cmp < 0)
	return -1;
      else if (cmp > 0)
	return 1;
      else{
	offset1 += strlen(key1 + offset1);
	offset2 += strlen(key2 + offset2);
	continue;
      }
    }
  }
  return 0;
}
