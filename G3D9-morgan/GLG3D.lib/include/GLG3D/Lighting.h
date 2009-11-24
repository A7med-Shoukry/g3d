

class Lighting : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Lighting> Ref;
private:

public:

    const Array<Light::Ref>& shadowedLightsArray() const;
    const Array<Light::Ref>& nonShadowedLightsArray() const;
    const Array<Light::Ref>& allLightsArray() const;

    /** Get all lights that potentially contribute at least \a cutoffRadiance to \a pos.*/
    void nearbyLights(const Vector3& pos, float cutoffRadiance, Array<Light::Ref>& lights) const;

    /** Cube map for use as the environment map.
        The MIP-map levels should be progressively blurred
        to represent varying powers of a cosine lobe.
        The lowest MIP level is cosine^1, i.e., perfectly
        diffuse reflection.
     */
    const Texture::Ref& environmentMap() const;
};
