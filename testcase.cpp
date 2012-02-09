#include <fstream>
#include <btree.hpp>


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
}
