#include<iostream>
#include<string>
#include<cstring>
#include<fstream>
#include<cstdlib>
#include<cstdio>
#include<cstring>
#include<queue>
#include "btree.hpp"
using namespace std;

typedef struct element_t{
  long long int offset;
  int parent;
  int level;
} element;

int main(){
  string s;
  cout << "Index name: " << endl;
  cin >> s;
  cout << "Fetching index" << endl;
  Index * ind = new Index(s.c_str());
  cout << "Fetching node now" << endl;
  node * root = ind -> fetchNode(BLOCKSIZE);
  cout << "Root Fetched" << endl;
  int pLen = ind -> getPayloadLen();
  // int kLen = ind -> getKeyLen();
  queue<element * > eQ;
  element * e;
  // root is node number 1 on level 1
  // 0 node does not exist
  int nNode = 1;
  cout << "Get Root" << endl;
  for(int i = 0;i < root -> numKeys;i++){
    e = new element;
    e -> offset = (long long int)root -> data[i * OFFSETSIZE];
    e -> parent = nNode;
    e -> level = 2;
    eQ.push(e);
  }
  cout << "Queue has an element" << endl;
  node * n;
  int p, l;
  while(!s.empty()){
    nNode++;
    e = eQ.front();
    eQ.pop();
    cout << 
    n = ind -> fetchNode(e -> offset);
    
    p = nNode;
    l = e -> level++;
    
    if(!n -> isLeaf){
      free(e);
      for(int i = 0; i < n -> numKeys + 1; i++){
	e = new element;
	e -> offset = (long long int)root -> data[i * OFFSETSIZE];
	e -> parent = p;
	e -> level = l;
	eQ.push(e);
      }
    } else{
      for(int i = 0;i < n -> numKeys;i++){
	cout << "Node no.: " << nNode << endl;
	cout << "Parent  : " << e -> parent << endl;
	cout << "Level   : " << l << endl;
	char * c = new char[pLen + 1];
	memcpy(c, (void * )(&root -> data[i * pLen]), 8);
	c[pLen] = '\0';
	cout << "Payload: " << endl;
	for(int i = 0;i < n -> numKeys;i++){
	  cout<< c[i] << endl;
	}
      }
      free(e);
    }
    ind -> delete_node(n);
  }
}
