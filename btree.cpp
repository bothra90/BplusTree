#include <btree.hpp>
#include <fstream>
#include <cmath> 

Index::Index(string _indexname, KeyType * _keytype, int _payloadLen){
  indexname = _indexname;
  keytype = _keytype;
  payloadLen = _payloadLen;
  keyLen = keyLength(KeyType);
  file.open(indexname.c_str(), ios::in|ios::binary|ios::ate | ios::out);
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
  ret -> keys = new char[ret -> numKeys * keyLen];
  ret -> data = new char[ret -> (numKeys + 1) * fmax(payloadLen, OFFSETSIZE)];
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
    file >> payloadLen;
    // Set maxKeys
    keyLen = keyLength(KeyType);
    maxKeys = (BLOCKSIZE - 1 - sizeof(int) - fmax(sizeof(payloadLen), OFFSETSIZE)) / ( keyLen + fmax(sizeof(payloadLen), OFFSETSIZE));
    // Load root node
    root = getRoot();
    root -> parent = NULL;
  }
  else{
    printf("Unable to open file\n");
  }
  file.close();
}


#define KEY(a) (a * keyLen)

/*Finds pointer to follow in a node using binary search on key*/
int Index::find_ptr(node * n, byte key[]){
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * key1,* currKey ;
  key1 = &(n -> keys[KEY(right)]);
  if(compareKeys(key, key1) > 0) return right + 1;
  while((left + 1)< right){
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
  
  while((left + 1)< right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey) > 0){
      left = current;
    }
    else{
      right = current;
    }
  }
  // current = (left + right) / 2;
  // Note: key is strictly greater than left key
  currKey = &(n -> keys[KEY(right)]);
  if(compareKeys(key, right) == 0){
    return current;
  }
  return -1;
}

/*Finds the place to store key*/   
int Index::find_prev_key(node * n, byte key[]){
  assert(n -> isLeaf);
  int left = 0, right = n -> numKeys - 1, current = n -> numKeys / 2;
  byte * currKey ;
  currKey = &(n -> keys[KEY(right)]);
  // If key is strictly gerater tha right key then key to be stored beyond leaf, requires splitting.
  if(compareKeys(key, currKey)) return right + 1;
  while((left + 1)< right){
    current = (left + right) / 2;
    currKey = &(n -> keys[KEY(current)]);
    if(compareKeys(key, currKey) > 0){
      left = current;
    }
    else{
      right = current;
    }
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
  if(root -> offset == NULL) return 0;
  node * curr = root,* prev ;
  long long int loc;
  while(!curr -> isLeaf){
    loc = *(long long int *) (&curr -> data[find_ptr(curr, key) * OFFSETSIZE]);
    file.seekg(loc);
    delete_nod(curr);
    curr = fetchNode(loc);
  }
  // assert(curr -> isLeaf);
  int loc = find_key(curr, key);
  if(loc == -1)
    return 0;
  else{
    memcpy(payload, &(curr -> data[loc * payloadLen]), payloadLen);
    return 1;
  }
}

node * Index::create_new_node(long long int _offset, bool _isLeaf){
  node * ret = new node;
  ret -> offset = _offset;
  ret -> numKeys = 0;
  ret -> isLeaf = _isLeaf;
  ret -> keys = new byte[maxKeys * keyLen];
  if(_isLeaf) ret -> data = new byte[maxKeys * payloadLen];
  else ret -> data = new byte[maxKeys * OFFSETSIZE];
  return ret;
}

// Adds an entry to a node
void Index::add_entry(node * n, byte * key, byte * payload){
  memcpy(&(n -> keys[n -> numKeys * keyLen]), key, keyLen);
  if(n -> isLeaf){
    memcpy(&(n -> data[n -> numKeys * payloadLen]), payload, payloadLen);
  }
  else{
    memcpy(&(n -> data[n -> numKeys * OFFSETSIZE]), payload, OFFSETSIZE);
  }
  n -> numKey++;
}

void Index::update_node(node * n){
  file.seekg(n -> offset);
  file << n -> isLeaf;
  file << n -> numKeys;
  for(int i = 0;i < n -> numKeys;i++){
    
  }
}

int Index::insert(byte key[], byte payload[]){
  if(root -> offset == NULL){
    // Root to be added
    file.seekg(BLOCKSIZE);
    // Create root node
    node * root = create_new_node(BLOCKSIZE);
    root -> numKeys = 1;
    add_entry(root, key, payload);
  }
    
  node * curr = root;
  long long int loc;
  while(!curr -> isLeaf){
    loc = *(long long int *) (&curr -> data[find_ptr(curr, key) * OFFSETSIZE]);
    file.seekg(loc);
    prev = curr;
    curr = fetchNode(loc);
    curr -> parent = prev;
  }
  // assert(curr -> isLeaf);
  int loc = find_prev_key(curr, key);
  
  node * newNode;
  if(loc == (maxKeys + 1)){
    // Create new node
    newNode = new node;
    // Store data in node construct
    split_leaf(curr, newNode);
    // Split Node
    // a. Copy data from current node to new node
    // b. Update keys count in current node
    // c. Dump new node
  }
    return 0;
  else{
    memcpy(payload, &(curr -> data[loc * payloadLen]), payloadLen);
    return 1;
  }
}
