#ifndef _BTREE_HPP_
#define _BTREE_HPP_

#include<string>
#include<cstring>
#include<cstdlib>
#include<cstdio>
#include<iostream>

#define INTSIZE 4
#define MAXATTRS 16
#define BLOCKSIZE 4096
#define OFFSETSIZE 8
typedef byte char;

enum attrType{intType, stringType};

typedef struct KeyType_t{
  int numAttrs;
  attrType attrTypes[MAXATTRS];
  int attrLen[MAXATTRS];
} KeyType;

typedef char byte;

typedef struct node_t{
  node * parent;
  long long int offset; // offset == 0 => node does not exist
  /*Following stored in index */   
  bool isLeaf;
  int numKeys;
  byte * keys;
  byte * data; // Payload / Block Pointers(Offsets)
} node;

int keylen		(KeyType *keytype);
int setIntAttrval	(byte *key, KeyType * keytype, int attrnum, int val);
int getIntAttrVal	(byte *key, KeyType * keytype, int attrnum, int& retval);
int setStringAttrVal	(byte *key, KeyType * keytype, int attrnum, char *val);
int getStringAttrVal	(byte *key, KeyType * keytype, int attrnum, char *retval);

class Index{

private:
  /* Index Variables*/ 
  KeyType * keytype;
  int payloadlen;
  string indexname;
  fstream file;
  int maxKeys;
  node * root;

  /*Helper Functions*/   
  node * getRoot();
  node * fetchNode(long long int offset);
  void delete_node(node * n);
  
public:
  /*
    It constructs an index in a file whose name is specified in indexname. You can assume a fixed directory for all files. The keytype and payloadlen information, along with a pointer to the file offset of the root block of the tree, should be stored as a struct copied into the head of the index. (Note that KeyType is of fixed length.)
  */   
  Index(string _indexname, KeyType * _keytype, int _payloadlen);
  /*
    Opens an existing index, and retrieves the associated type/length information from the head of the index.  
   */
  Index(string _indexname);
  /*
    Key comparison should be based on the type of the key specified earlier, and it should be easy to modify your code to add new types. You do NOT have to make this object oriented, you just need a switch on the key type insider your insert/lookup functions.  The index should only support non-duplicate keys. If a key is duplicated, insert should return 0, and not insert the duplicate. Otherwise a value of 1 is returned. */ 
  int insert(byte key[], byte payload[]);
  /*
    Payload should be an array of bytes of the required size in which
the retrieved payload is stored. The return value is 1 if the value is found, and 0 otherwise. */ 
  int lookup(byte key[], byte payload[]); 
};
#endif
