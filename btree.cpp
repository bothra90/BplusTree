#include "btree.hpp" 
#include <cmath>
#include <cassert>
#include <iostream>


using namespace std;

Index::Index(string _indexname, KeyType * _keytype, int _payloadLen){
  indexname = _indexname;
  keytype = _keytype;
  payloadLen = _payloadLen;
  keyLen = keyLength(keytype);
  file.open(indexname.c_str(), fstream::in | fstream::out | fstream::binary | fstream::trunc);
  assert(file.is_open());
  int datasz = fmax(sizeof(payloadLen), OFFSETSIZE);
  maxKeys = (BLOCKSIZE - 1 - sizeof(int) - datasz) / (keyLen + datasz);
  // Store keytype in first block
  file << keytype -> numAttrs;
  for(int i = 0;i < keytype -> numAttrs;i++){
    file << keytype -> attrTypes[i];
  }
  for(int i = 0;i < keytype -> numAttrs;i++){
    file << keytype -> attrLen[i];
  }
  file << payloadLen;
  root = new node;
  create_new_node(BLOCKSIZE, 1, &root);
  file << (long long int)BLOCKSIZE;
  block_multiple();
  update_node(&root);
  block_multiple();
}

Index::Index(string _indexname){
  indexname = _indexname;
  file.open(indexname.c_str(), ios::in|ios::binary| ios::out); 
  keytype = new KeyType;
  if (file.is_open()){
    file >> keytype->numAttrs;
    int type;
    for(int i = 0;i < keytype->numAttrs;i++){
      file >> type;
      keytype -> attrTypes[i] =(attrType) type;
    }
    for(int i = 0;i < keytype->numAttrs;i++){
      file >> keytype->attrLen[i];
    }
    file >> payloadLen;
    // Set maxKeys
    keyLen = keyLength(keytype);
    maxKeys = (BLOCKSIZE - 1 - sizeof(int) - fmax(sizeof(payloadLen), OFFSETSIZE)) / ( keyLen + fmax(sizeof(payloadLen), OFFSETSIZE));
    // Load root node
    root = getRoot();
    root -> parent = NULL;
  }
  else{
    printf("Unable to open file\n");
  }
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
  ret -> keys = new char[ret -> numKeys * keyLen];
  ret -> data = new char[((ret -> numKeys )+ 1 )* max(payloadLen, OFFSETSIZE)];
  for(int i = 0;i < ret -> numKeys * keyLen;i++){
    file >> ret -> keys[i];
  }
  for(int i = 0;i < (ret -> numKeys + 1)* fmax(payloadLen, OFFSETSIZE);i++){
    file >> ret -> data[i];
  }
  return ret;
}


void Index::delete_node(node * n){
  free(n -> keys);
  free(n -> data);
  free(n);
  return;
}

void Index::create_new_node(long long int _offset, char _isLeaf, node **dst){
  node * ret = (node *)malloc(sizeof(node));
  ret -> offset = _offset;
  ret -> numKeys = 0;
  ret -> isLeaf = _isLeaf;
  ret -> keys = (char *)malloc(maxKeys * keyLen);
  int datasz = _isLeaf? payloadLen : OFFSETSIZE;
  ret -> data = (char *)malloc((maxKeys + 1) * datasz);
  *dst = ret;
}


/*
   Transfer the constructed node to its assigned position on the file
   */   
void Index::update_node(node ** np){
  node * n = *np;
  assert(file.is_open());
  int datasz = n -> isLeaf ?  payloadLen :  OFFSETSIZE;
  file.seekg(n -> offset);
  file << n -> isLeaf;
  file << n -> numKeys;
  for(int i = 0;i < (n -> numKeys) * keyLen;i++){
    file << n -> keys[i];
  }
  for(int i = 0;i < (n -> numKeys + 1) * datasz;i++){
    file << n -> data[i];
  }
}

void Index::assert_filesz(){
  file.seekg(0,ios::end);
  int length = file.tellg();
  assert((length) % BLOCKSIZE == 0);
}


/*
   Aligns given node to block size in file
   */ 
void Index::block_multiple(){
  file.seekg(0,ios::end);
  int length = file.tellg();
  int sz = BLOCKSIZE - (length) % BLOCKSIZE;
  for (int i=0; i< sz; i++)
    file << " ";
  assert_filesz();
}

byte * Index::split_node(node * src, node ** dstp){
  // Assumes src is a valid node
  node * dst = *dstp;
  int datasz = src->isLeaf? payloadLen : OFFSETSIZE;
  file.seekg(0,ios::end);
  long long int end = file.tellg();
  create_new_node(end, src -> isLeaf, &dst);
  int split = maxKeys / 2;
  for(int i = split;i < maxKeys;i++ ){
    memcpy(&((dst->keys)[(i - split)* keyLen]),&((src->keys)[i * keyLen]), keyLen);
    memcpy(&((dst->data)[(i - split)* datasz]),&((src->data)[i * datasz]), datasz);
  }
  memcpy(&(dst->data[maxKeys - split]),&(src->data[maxKeys]),OFFSETSIZE);
  memcpy(&(src->data[split]), &(dst->offset), OFFSETSIZE);
  dst -> numKeys = maxKeys - split;
  src -> numKeys = split;
  *dstp = dst;
  return dst -> keys;
}

#define KEY(a) (a * keyLen)

/*Finds the place to store key*/   
int Index::find_next_key(node * n, byte key[]){
  if (n -> numKeys == 0)
    return 0;
  int leftIndex = 0, rightIndex = n -> numKeys - 1, currIndex = (n -> numKeys) / 2;
  byte *currKey, *leftKey, *rightKey;
  rightKey = &(n->keys[KEY(rightIndex)]);
  leftKey  = &(n->keys[KEY(leftIndex)]);
  if(compareKeys(leftKey, key, keytype) > 0) return leftIndex;
  else if(compareKeys(rightKey, key, keytype) < 0) return rightIndex + 1;
  else{
    while(leftIndex + 1 < rightIndex){
      currIndex = (leftIndex + rightIndex) / 2;
      currKey = &(n -> keys[KEY(currIndex)]);
      int com_median = compareKeys(currKey, key, keytype);
      if(com_median == 0){
	return currIndex;
      }
      else if(com_median < 0){
	leftIndex = currIndex;
      }
      else{
	rightIndex = currIndex;
      }
    }
    return rightIndex;
  }
}

void Index::insert_key_in_node(node * currNode, byte * _key, byte * _data){
  //_data goes to the left of _key in the currNode
  char isLeaf = currNode -> isLeaf;
  int datasz = isLeaf ? payloadLen : OFFSETSIZE;
  int keyIndex = find_next_key(currNode, _key);
  memcpy(&(currNode -> data[(currNode -> numKeys + 1)* datasz]), &(currNode -> data[(currNode -> numKeys) * datasz]), datasz);
  for(int i = (currNode -> numKeys - 1);i >= keyIndex;i--){
    memcpy(&(currNode -> keys[(i + 1)* keyLen]),&(currNode -> keys[i * keyLen]), keyLen);
    memcpy(&(currNode -> data[(i + 1)* datasz]),&(currNode -> data[i * datasz]), datasz);
  }
  memcpy(&(currNode -> keys[keyIndex * keyLen]), _key, keyLen);
  memcpy(&(currNode -> data[keyIndex * datasz]), _data, datasz);
  currNode -> numKeys += 1;
  if(currNode -> numKeys == maxKeys){
    cout << "I want to split now" << endl;
    node * newNode = NULL;
    byte * newKey;
    newKey = split_node(currNode, &newNode);
    newNode -> parent = currNode -> parent;
    update_node(&newNode); // Updates node in file
    block_multiple(); //Makes file blocksz multiple
    byte * d = new byte[OFFSETSIZE];
    if(currNode -> parent == NULL){
      file.seekg(0,ios::end);
      long long int end = file.tellg();
      create_new_node(end, 0, &root);
      currNode -> parent = root;
      newNode -> parent = root;
      //set left pointer to point to currNode
      memcpy(root->data,&(newNode->offset),OFFSETSIZE);
      memcpy(d, &(currNode -> offset), OFFSETSIZE); //d points to newNode
      insert_key_in_node(root, &(newNode -> keys[KEY(0)]) , d);
      update_node(&root);
      block_multiple(); //Makes file blocksz multiple
    }
    else{
      memcpy(d, &(newNode -> offset), OFFSETSIZE); //d points to newNode
      insert_key_in_node(currNode -> parent, newKey, d);
    }

  }
  update_node(&currNode);
}

int Index::insert(byte key[], byte payload[]){
  assert(file.is_open());
  node * currNode = root;
  // Root DNE
  long long int loc;
  int currIndex;
  while(!currNode -> isLeaf){
    currIndex = find_next_key(currNode, key);
    loc = *(long long int *) (&currNode -> data[currIndex * OFFSETSIZE]);
    file.seekg(loc);
    currNode = fetchNode(loc);
  }
  insert_key_in_node(currNode, key, payload);
  return 1;
}

int Index::lookup(byte key[], byte payload[]){
  if(root -> offset == 0){
    cout << "No entries in index."<< endl;
    return 0;
  }
  node * curr = root,* prev ;
  long long int loc;
  while(!curr -> isLeaf){
    loc = *(long long int *) (&curr -> data[find_next_key(curr, key) * OFFSETSIZE]); //FIXED
    file.seekg(loc);
    curr = fetchNode(loc);
  }
  // assert(curr -> isLeaf);
  int index = find_next_key(curr, key);
  if (compareKeys(curr->keys + index * payloadLen, key, keytype) != 0)
    return 0;
  else{
    memcpy(payload, &(curr -> data[index * payloadLen]), payloadLen);
    return 1;
  }
}

void Index::closeIndex(){
  file<<flush;
  file.close();
}
