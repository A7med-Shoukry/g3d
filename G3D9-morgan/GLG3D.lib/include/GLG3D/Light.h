
/** Point light source: spot, omni, or directional light */
class Light : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Light> Ref;

protected:

   
public:

    /** Coordinate frame of the light. */
    const CFrame& cframe() const;
    void setCFrame(const CFrame& c);

    /** Total power of the light, in Watts. */
    Color3 power() const;
    void setPower(const Color3& p);

    /** The attenuation function is 1 / (a[0] + a[1] r + a[2] r^2).  
    Physically correct falloff is a = (0,0,1). */
    const Vector3& attenuationCoefficients() const;
    void setAttenuationCoefficients(const float a[3]);
    void setAttenuationCoefficients(const Vector3& a);
    void setAttenuationCoefficients(float a0, float a1, float a2);

    /** Spot light cutoff angle in radians. */
    float spotCutoffAngle() const;
    void setSpotCutoffAngle(float a);

    enum SpotShape {CIRCLE, SQUARE};

    /** Spot lights can have either a round or square cutoff. */
    SpotShape spotCutoffShape() const;
    void setSpotCutoffShape(SpotShape s);

    /** Lights can be disabled without affecting their other properties. */
    bool enabled() const;
    void setEnabled(bool b);

    /** True if the light casts shadows */
    bool castsShadows() const;
    void setCastsShadows(bool b);

    /** \param h if -1, the w value is used */
    void setShadowMapSize(int w, int h = -1);
    void getShadowMapSize(int& w, int& h) const;

    /** If this light casts shadows, this is the shadow map for it. Allocated on demand 
        if the light casts shadows.*/
    ShadowMap::Ref shadowMap() const;
    void updateShadowMap(...);

};
