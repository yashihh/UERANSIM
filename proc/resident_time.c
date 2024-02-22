#include "resident_time.h"

int resident_time = 0;

int get_Resident_time(void){
    return resident_time;
}
void set_Resident_time(int val){
    resident_time = val;
}