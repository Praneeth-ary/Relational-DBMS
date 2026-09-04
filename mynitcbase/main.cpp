#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {
    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;
    return FrontendInterface::handleFrontend(argc,argv);
}

