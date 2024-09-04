
#include "STLNonManifold.h"
#include "argparser.hpp"



int main(int argc, char const* argv[])
{
    //STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/zhu.stl");
    //STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker("./stl_models/Stanford_Bunny_sample.stl");

    auto args_parser = util::argparser("STLNonManifold");
    args_parser.add_help_option()
        .use_color_error()
        .add_option<std::string>("-o", "--output", "output obj path", "./output_obj.obj")
        .add_argument<std::string>("stl_model_path", "stl model path")
        .parse(argc, argv);

    std::string output_obj_path = args_parser.get_option<std::string>("-o");
    std::string stl_model_path = args_parser.get_argument<std::string>("stl_model_path");

    //std::string output_obj_path = "./output_obj.obj";
    //std::string stl_model_path = 


    STLNonManifold::STLNonManifoldChecker stlNonManifoldChecker(stl_model_path);
    stlNonManifoldChecker.CheckNonManifold();
    stlNonManifoldChecker.Export2OBJ(output_obj_path);
}
