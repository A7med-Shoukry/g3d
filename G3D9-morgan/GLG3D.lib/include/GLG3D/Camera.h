class Camera {
public:
    enum Type {ORTHOGRAPHIC, PERSPECTIVE, CUSTOM};

    /** Camera-to-world matrix */
    const CFrame& cframe() const;
    const CFrame&  worldToCameraMatrix() const;
    const Matrix4& projectionMatrix(int screenWidth, int screenHeight) const;
};
