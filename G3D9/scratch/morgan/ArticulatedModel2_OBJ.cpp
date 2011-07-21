#include "ArticulatedModel2.h"



void ArticulatedModel2::loadOBJ(const Specification& specification) {
    ParseOBJ parser;
    parser.parse(TextInput(specification.filename));
}

