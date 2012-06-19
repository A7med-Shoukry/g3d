/**
 \file GLG3D/Shader2.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited 2012-06-13

 TODO: Add parameter to turn off preprocessor to Specification



 */

#include "G3D/platform.h"
#include "GLG3D/Shader2.h"
namespace G3D {


bool s_failureBehavior;


Shader2::Specification::Specification(){}

Shader2::Specification::Specification(const Any& any){
    /* if(any.containsKey("vertexFile")){
        vertex.val  = any["vertexFile"].string();
        vertex.type = FILE;
    } else if(any.containsKey("vertexString")){
        vertex.val = any["vertexString"].string();
    }
    if(any.containsKey("tessellationFile")){
        tessellation.val  = any["tessellationFile"].string();
        tessellation.type = FILE;
    } else if(any.containsKey("tessellationString")){
        tessellation.val = any["tessellationString"].string();
    }
    if(any.containsKey("tessellationControlFile")){
        tessellationControl.val  = any["tessellationControlFile"].string();
        tessellationControl.type = FILE;
    } else if(any.containsKey("tessellationControlString")){
        tessellationControl.val = any["tessellationControlString"].string();
    } 
    if(any.containsKey("geometryFile")){
        geometry.val  = any["geometryFile"].string();
        geometry.type = FILE;
    } else if(any.containsKey("geometryString")){
        geometry.val = any["geometryString"].string();
    } 
    if(any.containsKey("pixelFile")){
        pixel.val  = any["pixelFile"].string();
        pixel.type = FILE;
    } else if(any.containsKey("pixelString")){
        pixel.val = any["pixelString"].string();
    } */
}

Shader2::Source::Source(SourceType t, const std::string& value) :
    type(t), val(value){}

Shader2::Source::Source(){

}

void Shader2::setFailureBehavior(FailureBehavior f){
    s_failureBehavior = f;
}
    
void Shader2::reload(){
       
}

bool Shader2::ok() const{
    return m_ok;
}

Shader2::Ref Shader2::create(const Specification& s){
    return new Shader2();
}

    



}