#ifndef __CONTEXTSWITCH_H__
#define __CONTEXTSWITCH_H__

#include <kernel/kernGlobal.h>

void 
kerent();

syscallRequest* 
kerxit(task* active);

#endif
