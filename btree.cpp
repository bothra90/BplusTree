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
  maxKeys = (BLOCKSIZE - 1 - sizeof(int) - fmax(sizeof(payloadLen), OFFSETSIZE)) / (keyLen + fmax(sizeof(payloadLen), OFFSETSIZE));
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
  file << (long long int)0; // Initial offset of NULL for root, i.e., no root yet.
  root -> offset = 0; //  set root offset as zero when there is no data in the index
  block_multiple();
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


#define KEY(a) (a * keyLen)

/*Finds pointer to follow in a node using binary search on key*/
int Index::find_ptr(node * n, byte key[]){
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * key1,* currKey ;
  key1 = &(n -> keys[KEY(right)]);
  if(compareKeys(key, key1,keytype) > 0) return right + 1;
  while((left + 1)< right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey,keytype) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  return left + 1;
}

/*Finds key in a leaf node using binary search
  Returns key number to insert at
 */  
// XXX why assume leaf? 
int Index::find_index(node * n, byte key[]){
  // assert(n -> isLeaf);
  int left = 0, right = (n -> numKeys) - 1, current = (n -> numKeys) / 2;
  byte * key1,* currKey ;
  while((left + 1)< right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey,keytype) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  if(DEBUG){
    cout << "index:" << *(int *)key <<","<< (char *)(key + 4) << "," << *(int *)(key + 12) << endl;
  }
  // current = (left + right) / 2;
  // Note: key is strictly greater than left key
  currKey = &(n -> keys[KEY(right)]);
  if(DEBUG){
    cout << "index:" << *(int *)currKey <<","<< (char *)(currKey + 4) << "," << *(int *)(currKey + 12) << endl;
  }
  if(compareKeys(key, currKey,keytype) == 0){
    return current;
  }
  return -1;
}

/*Finds the place to store key*/   
int Index::find_next_key(node * n, byte key[]){
  //assert(n -> isLeaf);
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * currKey ;
  currKey = &(n -> keys[KEY(right)]);
  // If key is strictly gerater tha right key then key to be stored beyond leaf, requires splitting.
  if(compareKeys(key, currKey,keytype)) return right + 1;
  while((left + 1)< right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey,keytype) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  if(DEBUG){
    cout << right << endl;
  }
  // New node to be stored on right node
  return right;
}

void Index::delete_node(node * n){
  free(n -> keys);
  free(n -> data);
  free(n);
  return;
}

int Index::lookup(byte key[], byte payload[]){
  if(root -> offset == 0){
    cout << "No entries in index."<< endl;
    return 0;
  }
  node * curr = root,* prev ;
  long long int loc;
  while(!curr -> isLeaf){
    loc = *(long long int *) (&curr -> data[find_ptr(curr, key) * OFFSETSIZE]);
    file.seekg(loc);
    delete_node(curr);
    curr = fetchNode(loc);
  }
  // assert(curr -> isLeaf);
  int index = find_index(curr, key);
  if(index == -1)
    return 0;
  else{
    memcpy(payload, &(curr -> data[index * payloadLen]), payloadLen);
    return 1;
  }
}

void Index::create_new_node(long long int _offset, char _isLeaf, node **dst){
  node * ret = (node *)malloc(sizeof(node));
  ret -> offset = _offset;
  ret -> numKeys = 0;
  ret -> isLeaf = _isLeaf;
  ret -> keys = (char *)malloc (maxKeys * keyLen);
  if(_isLeaf) ret -> data = (char *)malloc(maxKeys * payloadLen);
  else ret -> data = (char *)malloc(maxKeys * OFFSETSIZE);
  *dst = ret;
}

// Adds an entry to a node . XXX makes it the rightmost entry
void Index::add_entry(node * n, byte * key, byte * _data){
  int datasz = (n -> isLeaf)? payloadLen : OFFSETSIZE; //FIXED
  memcpy(&(n -> keys[n -> numKeys * keyLen]), key, keyLen);
  if(n -> isLeaf){
    memcpy(&(n -> data[n -> numKeys * datasz]), _data, datasz);
  }
  else{
    memcpy(&(n -> data[n -> numKeys * OFFSETSIZE]), _data, OFFSETSIZE);
  }
  n -> numKeys++;
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
  dst -> numKeys = maxKeys - split;
  src -> numKeys = split;
  *dstp = dst; // FIXED here
  return dst -> keys;
}

void Index::insert_key_in_node(node * currNode, byte * _key, byte * _data){
  //FIXED
  if (currNode -> numKeys == 0){
    int datasz;
    add_entry(currNode, _key, _data);
  }
  if(DEBUG){
    cout << "In insert"<< _data << endl;
  }
  if(DEBUG){
    cout <<"here1" << endl;
  } 
  char isLeaf = currNode -> isLeaf;
  if(DEBUG){
    cout <<"here2" << endl;
  } 
  int keyIndex;
  int datasz = isLeaf ? payloadLen : OFFSETSIZE;
  keyIndex = find_next_key(currNode, _key);
  byte * tempKey = new byte[keyLen];
  for(int j = 0;j < datasz ;j++){
    currNode -> data[(currNode -> numKeys + 1)* datasz + j] = currNode -> data[(currNode -> numKeys) * datasz + j];
  }
  for(int i = (currNode -> numKeys - 1);i >= keyIndex;i--){
    for(int j = 0;j < keyLen;j++){
      currNode -> keys[(i + 1)* keyLen + j] = currNode -> keys[i * keyLen + j];
    }
    for(int j = 0;j < datasz;j++){
      currNode -> data[(i + 1)* datasz + j] = currNode -> data[i * datasz + j];
    }
  }
  if(DEBUG){
    cout << "insert:" << *(int *)_key <<","<< (char *)(_key + 4) << "," << *(int *)(_key + 12) << endl;
    cout <<"key Index for new data" << _data << endl << keyIndex << endl;
  }
  memcpy(&(currNode -> keys[keyIndex * keyLen]), _key, keyLen);
  if(DEBUG){
    cout << "insert:" << *(int *)_key <<","<< (char *)(_key + 4) << "," << *(int *)(_key + 12) << endl;
  }
  memcpy(&(currNode -> data[keyIndex * datasz]), _data, datasz);
  if(DEBUG){
    cout << "insert:" << *(int *)_key <<","<< (char *)(_key + 4) << "," << *(int *)(_key + 12) << endl;
  }
  currNode -> numKeys += 1;
  // Create new node at end of file

  // Store data in node construct
  // Split Node
  // a. Copy data from current node to new node
  // b. Update keys count in current node
  // _key contains first key of newnode
  // newnode contains
  // c. Dump new node

  node * newNode = NULL;
  byte * newKey;

  if(currNode -> numKeys == maxKeys){
    newKey = split_node(currNode, &newNode);
    update_node(&newNode); // Updates node in file
    block_multiple(); //Makes file blocksz multiple
    byte * d = new byte[OFFSETSIZE];
    memcpy(d, &(newNode -> offset), OFFSETSIZE);
    //TODO seems currNode->parent is not set
    if(currNode -> parent == NULL){
      long long int end = file.tellg();
      create_new_node(end, 0, &currNode->parent); //XXX changed. but not working
      root = currNode -> parent; //
    }
    insert_key_in_node(currNode -> parent, newKey, d);
  }
}

int Index::insert(byte key[], byte payload[]){
  assert(file.is_open());
  if(root -> offset == 0){
    // Root to be added
    file.seekg(BLOCKSIZE);
    // Create root node
    create_new_node(BLOCKSIZE, 1, &root);
    add_entry(root, key, payload);
    update_node(&root);
    block_multiple();
    return 1;
  }
  // Root exists
  node * prev, * currNode = root;
  long long int loc;
  // vetor < int > chosen;
  int temp;
  while(!currNode -> isLeaf){
    temp = find_ptr(currNode, key);

    // chosen -> push_back(temp)
    loc = *(long long int *) (&currNode -> data[temp * OFFSETSIZE]);
    file.seekg(loc);
    prev = currNode;
    currNode = fetchNode(loc);
    currNode -> parent = prev;
  }
  // assert(currNode -> isLeaf);
  insert_key_in_node(currNode, key, payload); //  1 for leaf
  // memcpy(payload, &(currNode -> data[loc * payloadLen]), payloadLen);
}

void Index::closeIndex(){
  file<<flush;
  file.close();
}
