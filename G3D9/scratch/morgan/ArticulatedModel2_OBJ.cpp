#include "ArticulatedModel2.h"


void ArticulatedModel2::loadOBJ(const Specification& specification) {
    ParseOBJ parseData;
    parseData.parse(TextInput(specification.filename));



}

