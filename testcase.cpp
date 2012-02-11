#include <fstream>
#include "btree.hpp"

void insert_key(byte * key, int a, int b, char c[8]){
  int offset = 0;
  memcpy(key + offset, &a, INTSIZE);
  offset += 4;
  memcpy(key + offset, c, 8);
  offset += 8;
  memcpy(key + offset, &b, INTSIZE);
  return;
}


int main(){
  KeyType * keytype = new KeyType;
  keytype->numAttrs = 3;
  keytype->attrTypes[0] = intType;
  keytype->attrTypes[1] = stringType;
  keytype->attrTypes[2] = intType;
  keytype->attrLen[0] = INTSIZE;
  keytype->attrLen[2] = INTSIZE;
  keytype->attrLen[1] = 7; // 8 character string
  int payloadLen = 2;
  Index * foo = new Index("foo",keytype, payloadLen);
  int keylen = keyLength(keytype);

  if(DEBUG){
    cout << foo->maxKeys << endl;
  }

  byte * key = new byte[keylen];
  char str[] = "this is";
  insert_key(key, 4, 12, str);
  char pay[] = "a";
  foo->insert(key,pay);

  byte * key1 = new byte[keylen];
  char str1[] = "awesome";
  insert_key(key1, 4, 12, str1);
  char pay1[] = "b";
  foo->insert(key1,pay1);

  byte * key2 = new byte[keylen];
  char str2[] = "awesom1";
  insert_key(key2, 4, 22, str2);
  char pay2[] = "c";
  foo->insert(key2,pay2);

  byte * key3 = new byte[keylen];
  char str3[] = "awesom2";
  insert_key(key3, 8, 32, str3);
  char pay3[] = "d";
  foo->insert(key3,pay3);

  byte * key4 = new byte[keylen];
  char str4[] = "awesom3";
  insert_key(key4, 1, 42, str4);
  char pay4[] = "e";
  foo->insert(key4,pay4);

  byte * key5 = new byte[keylen];
  char str5[] = "awesom4";
  insert_key(key5, 16, 52, str5);
  char pay5[] = "f";
  foo->insert(key5,pay5);


/*  cout << "main: " << *(int *)key1 << ","<< *(int *)key <<","<< (char *)(key1 + 4) <<","<<(char *)(key + 4) << ","<<*(int *)(key1 + 12)\
    << "," << *(int *)(key + 12) << endl; */
  // cout << "Comparing in Test :" << compareKeys(key2,key3,keytype) << endl;
  // cout << (char *)(key1 + 4) << " "<< (char *)(key + 4) << endl;
/*
  byte * res= new byte;
  foo->lookup(key3,res);
  cout << res[0] << endl;

  foo->lookup(key,res);
  cout << res[0] << endl;

  foo->lookup(key5,res);
  cout << res[0] << endl;
*/
  foo->closeIndex();
  return 0;
}
