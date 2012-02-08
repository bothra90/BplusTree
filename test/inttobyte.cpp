#include <iostream>
#include <stdlib.h>
using namespace std;
typedef char byte ;

int main(void){
byte byteArray[4];
int *b = (int *)byteArray;
*b = 20;
cout << *b << endl;
return 0;
}
