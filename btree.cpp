#include <btree.hpp>
#include <fstream>
#include <cmath> 

Index::Index(string _indexname, KeyType * _keytype, int _payloadlen){
  indexname = _indexname;
  keytype = _keytype;
  payloadlen = _payloadlen;
  file.open(indexname.c_str(), ios::in|ios::binary|ios::ate | ios::out);
  maxKeys = (BLOCKSIZE - 1 - sizeof(int) - fmax(sizeof(payloadlen), 8)) / ( + keylen(KeyType) + fmax(sizeof(payloadlen), 8));
  // Store keytype in first block
  file << keytype -> numAttrs;
  for(int i = 0;i < keytype -> numAttrs;i++){
    file << keytype -> attrTypes[i];
  }
  for(int i = 0;i < keytype -> numAttrs;i++){
    file << keytype -> attrLen[i];
  }
  file << payloadlen;
  root = new node;
  file << (long long int)0; // Initial offset of NULL for root, i.e., no root yet.
  root -> offset = 0;
}

node * Index::getRoot(){
  long long int offset;
  file.seekg(sizeof(int) + keytype -> numAttrs * ( sizeof(int) + sizeof(attrType)) + sizeof(int));
  file >> offset;
  return fetchNode(offset);
}

node * Index::fetchNode(long long int offset){
  file.seekg(offset);
  node * ret = new node;
  ret -> offset = offset;
  file >> ret -> isLeaf;
  file >> ret -> numKeys;
  ret -> keys = new char[ret -> numKeys * keylen(keytype)];
  ret -> data = new char[ret -> (numKeys + 1) * fmax(payloadlen, 8)];
  for(int i = 0;i < ret -> numKeys * keylen(keytype);i++){
    file >> ret -> keys[i];
  }
  for(int i = 0;i < (ret -> numKeys + 1)* fmax(payloadlen, 8);i++){
    file >> ret -> data[i];
  }
  return ret;
}

Index::Index(string _indexname){
  indexname = _indexname;
  file.open (indexname.c_str(), ios::in|ios::binary|ios::ate | ios::out);
  keytype = new KeyType;
  if (file.is_open()){
    file >> keytype->numAttrs;
    for(int i = 0;i < keytype->numAttrs;i++){
      file >> keytype->attrTypes[i];
    }
    for(int i = 0;i < keytype->numAttrs;i++){
      file >> keytype->attrLen[i];
    }
    file >> payloadlen;
    // Set maxKeys
    maxKeys = (BLOCKSIZE - 1 - sizeof(int) - fmax(sizeof(payloadlen), 8)) / ( + keylen(KeyType) + fmax(sizeof(payloadlen), 8));
    // Load root node
    root = getRoot();
    root -> parent = NULL;
  }
  else{
    printf("Unable to open file\n");
  }
  file.close();
}

int Index::insert(byte key[], byte payload[]){
}

#define KEY(a) (a * keylen(keytype))

/*Finds pointer to follow in a node using binary search on key*/
int Index::find_ptr(node * n, byte key[]){
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * key1,* currKey ;
  key1 = &(n -> keys[KEY(right)]);
  if(compareKeys(key, key1) > 0) return right + 1;
  while(left < right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  return left + 1;
}

/*Finds key in a leaf node using binary search*/
int Index::find_key(node * n, byte key[]){
  assert(n -> isLeaf);
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * key1,* currKey ;
  
  while(left < right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  current = (left + right) / 2;
  currKey = &(n -> keys[KEY(current)]);
  if(compareKeys(key, currKey) == 0){
    return current;
  }
  return -1;
}

void delete_node(node * n){
  free(n -> keys);
  free(n -> data);
  free(n);
  return;
}

int Index::lookup(byte key[], byte payload[]){
  if(root -> offset == NULL) return 0;
  node * curr = root;
  long long int loc;
  while(!curr -> isLeaf){
    loc = *(long long int *) (&curr -> data[find_ptr(curr, key) * OFFSETSIZE]);
    file.seekg(loc);
    delete_node(loc);
    curr = fetchNode(loc);
  }
  // assert(curr -> isLeaf);
  int loc = find_key(curr, key);
  if(loc == -1)
    return 0;
  else{
    memcpy(payload, &(curr -> data[loc * payloadlen]), payloadlen);
    return 1;
  }
}
