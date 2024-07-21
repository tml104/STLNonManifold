#include <iostream>

#include "STLNonManifold.h"

int main()
{
    STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/zhu.stl");
    stlNonManifoldChecker.CheckNonManifold();
}
