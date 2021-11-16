#include <stdint.h>
#include <stdlib.h>
#include "RVCOS.h"

void WriteString(const char *str){
    const char *Ptr = str;
    while(*Ptr){
        Ptr++;
    }
    RVCWriteText(str,Ptr-str);
}

int main(){
    uint32_t MSTickPerTick;
    TTick CurrentTicks, StartTicks, SleepTicks;

    WriteString("Main Thread: Starting\n");
    RVCTickMS(&MSTickPerTick);
    SleepTicks = 5000 / MSTickPerTick;
    RVCTickCount(&StartTicks);
    RVCThreadSleep(SleepTicks);
										    //WriteString("added Main Thread: Awake\n");
										    //while(1);
    RVCTickCount(&CurrentTicks);
    WriteString("Main Thread: Awake\n");
    											
    if(CurrentTicks < StartTicks + SleepTicks){
        WriteString("Main Thread: Woke too soon!\n");
        										//while(1);
        return 1;
    }
    WriteString("Main Thread: Exiting\n");
											//while(1);
    return 0;
}
