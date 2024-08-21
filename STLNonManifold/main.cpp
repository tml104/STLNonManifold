#include <iostream>

#include "STLNonManifold.h"

int main(int argc, char** argv)
{
    //STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/zhu.stl");
    //STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/Stanford_Bunny_sample.stl");

    STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker(argv[1]);
    stlNonManifoldChecker.CheckNonManifold();
}
