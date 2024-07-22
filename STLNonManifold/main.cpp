#include <iostream>

#include "STLNonManifold.h"

int main()
{
    STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/zhu.stl");
    //STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/Stanford_Bunny_sample.stl");
    
    stlNonManifoldChecker.CheckNonManifold();
}
