#include <fstream>
#include <btree.hpp>

void insert_key(byte * key, int a, int b, char c[8]){
  int offset = 0;
  memcpy(key + offset, a, INTSIZE);
  offset += 4;
  memcpy(key + offset, c, 8);
  offset += 8;
  memcpy(key + offset, b, INTSIZE);
  return;
}


int main(){
  key = new KeyType;
  key->numAttrs = 3;
  key->attrTypes = new attrType[3];
  key->attrTypes[0] = intType;
  key->attrTypes[1] = stringType;
  key->attrTypes[2] = intType;
  key->attrLen = new int[3];
  key->attrLen[0] = INTSIZE;
  key->attrLen[2] = INTSIZE;
  key->attrLen[1] = 8; // 8 character string
  int payloadLen = 1;
  Index foo = new Index("foo",key, payloadLen);
  int keylen = keyLength(key);
  byte key[] = new byte[keylen];
  insert_key(key, 4, 12, "this is ");
  foo.insert(key,'a');
  byte * res= new byte;
  foo.lookup(key,res);
}
