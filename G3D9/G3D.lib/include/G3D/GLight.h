/**
  @file GLight.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-11-12
  @edited  2010-09-01
*/

#ifndef G3D_GLight_h
#define G3D_GLight_h

#include "G3D/platform.h"
#include "G3D/Vector4.h"
#include "G3D/Vector3.h"
#include "G3D/Color4.h"

namespace G3D {
class Any;

/**
   A point (omni/spot/directional) light representation inspired by
   the classic OpenGL light format.

   A directional light has position.w == 0.  A spot light has
   spotHalfAngle < pi() / 2 and position.w == 1.  An omni light has
   spotHalfAngle == pi() and position.w == 1.
   
   Named "GLight" instead of "Light" so that you can define your
   own more general (e.g., area) emitter class without a name 
   conflict.

   \section lm Lighting Model

 <pre>
 r = distance to light (maybe inf)
 w_i = direction to light

 if local light:
     Phi = light.color  // J
     E_perp = Phi / (4 * pi * r^2) // W/m^2
     if spot light and X outside cone:
         E_perp = 0
 else: // infinite/directional light
     E_perp = E_L

 if in shadow:
     E_perp = 0

 // Adjust for angle of incidence
 E = E_perp * max(0, w_i dot n)
</pre>

  \sa G3D::SuperShader
 */
class GLight  {
public:
    /** World space position (for a directional light, w = 0) */
    Vector4             position;

    /** For a spot or directional light, this is the "right vector"
        that will be used when constructing a reference frame(). */
    Vector3             rightDirection;

    /** Direction in which the light faces, if a spot light.  This is
        the "look vector" of the light source. */
    Vector3             spotDirection;

    /** Spotlight cutoff half-angle in <B>radians</B>.  pi() = no
        cutoff (point/dir).  Values less than pi()/2 = spot light */
    float               spotHalfAngle;

    /** If true, G3D::SuperShader will render a cone of light large
        enough to encompass the entire square that bounds the cutoff
        angle. This produces a square prism instead of a cone of light
        when used with a G3D::ShadowMap.  for an unshadowed light this
        has no effect.*/
    bool                spotSquare;

    /** Constant, linear, quadratic */
    float               attenuation[3];

    /** 
        Point light: this is the the total power (\f$\Phi\f$) emitted
        uniformly over the sphere.  The incident normal irradiance at
        a point distance \f$r\f$ from the light is \f$ E_{\perp} =
        \frac{\Phi}{4 \pi r^2} \f$.
        
        Spot light: the power is the same as for a point light, but
        line of sight is zero outside the spot cone.  Thus the area
        within the spot cone does not change illumination when the
        cone shrinks.

        Directional light: this is the incident normal irradiance
        in the light's direction, \f$E_\perp\f$.
    */
    Color3              color;

    /** If false, this light is ignored */
    bool                enabled;

    bool                castsShadows;


    GLight();
   
    /** Accepted forms:
        - GLight::directional( vector3, color3, [bool])
        - GLight::spot(vector3, vector3, #, color3, [#, [#, [#, [#, [bool]]])
        - GLight::point(vector3, color3, [#, [#, [#, [#, [bool]]])
        - GLight { [all fields] }
    */
    GLight(const Any& any);

    /** For a point emitter, the actual power emitted, taking the
     spotlight cone into effect.  Undefined for a directional
     emitter. */
    Power3 power() const;
    
    /** Converts the Color3 to an Any. */
    operator Any() const;

    /** @param toLight will be normalized */
    static GLight directional(const Vector3& toLight, const Radiance3& color, bool castsShadows = true);

    static GLight point(const Vector3& pos, const Power3& color, float constAtt = 0.01f, float linAtt = 0, float quadAtt = 1.0f, bool castsShadows = true);

    /** @param pointDirection Will be normalized.  Points in the
        direction that light propagates.

        @param cutOffAngleDegrees Must be on the range [0, 90]. This
        is the angle from the point direction to the edge of the light
        cone.  I.e., a value of 45 produces a light with a 90-degree 
        cone of view.
    */
    static GLight spot(const Vector3& pos, const Vector3& pointDirection, float halfAngleRadians, 
                       const Color3& color, float constAtt = 0.01f, float linAtt = 0, float quadAtt = 1.0f,
                       bool castsShadows = true);

    /** Creates a spot light that looks at a specific point (by calling spot() ) */
    static GLight spotTarget(const Vector3& pos, const Vector3& target, float halfAngleRadians, 
                       const Color3& color, float constAtt = 0.01f, float linAtt = 0, float quadAtt = 1.0f,
                       bool castsShadows = true) {
           return spot(pos, target - pos, halfAngleRadians, color, constAtt, linAtt, quadAtt, castsShadows);
    }

    /** Returns the sphere within which this light has some noticable effect.  May be infinite.
        @param cutoff The value at which the light intensity is considered negligible. */
    class Sphere effectSphere(float cutoff = 30.0f / 255) const;

    /** Computes a reference frame (e.g., for use with G3D::ShadowMap */
    class CoordinateFrame frame() const;

    bool operator==(const GLight& other) const;
    bool operator!=(const GLight& other) const;

    /** Distance from the point to the light (infinity for directional lights) */
    float distance(const Point3& p) const {
        return (p - position.xyz()).length() / position.w;
    }
};

} // namespace
#endif

