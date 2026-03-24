#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

namespace bStream { class CStream; }

class J3DTransformInfo {
    glm::vec3 mScale = { 1.0f, 1.0f, 1.0f };
    glm::quat mRotation = glm::quat_identity<float, glm::defaultp>();
    glm::vec3 mTranslation = { 0.0f, 0.0f, 0.0f };
    
    mutable glm::mat4 mSRTMatrix = glm::identity<glm::mat4>();
    mutable bool mSRTDirty = false;

	void CalculateSRTMatrix() const;

public:
	J3DTransformInfo() = default;
    J3DTransformInfo(const J3DTransformInfo& other) = default;
    J3DTransformInfo(J3DTransformInfo&& other) noexcept = default;

	J3DTransformInfo& operator=(J3DTransformInfo const& info) {
		mScale       = info.mScale;
		mRotation    = info.mRotation;
		mTranslation = info.mTranslation;
        mSRTMatrix   = info.mSRTMatrix;
		return *this;
	}

	inline const glm::vec3& GetScale() const { return mScale; }
    inline const glm::quat& GetRotation() const { return mRotation; }
    inline const glm::vec3& GetTranslation() const { return mTranslation; }

    inline void SetScale(const glm::vec3& scale) {
        mScale = scale;
        mSRTDirty = true;
    }

    inline void SetRotation(const glm::quat& rotation) {
        mRotation = rotation;
        mSRTDirty = true;
    }

    inline void SetTranslation(const glm::vec3& translation) {
        mTranslation = translation;
        mSRTDirty = true;
    }

    inline void SetSRT(const glm::vec3& s, const glm::quat& r, const glm::vec3& t) {
        mTranslation = t;
        mRotation = r;
        mScale = s;
        mSRTDirty = true;
    }

	inline const glm::mat4& ToMat4() const {
        if (mSRTDirty) {
            CalculateSRTMatrix();
        }
        return mSRTMatrix;
    }

	void Deserialize(bStream::CStream* stream);

	bool operator==(const J3DTransformInfo& other) const;
	bool operator!=(const J3DTransformInfo& other) const;
};

class J3DTextureSRTInfo {
    glm::vec2 mScale = { 1.0f, 1.0f };
	float mRotation = 0.0f;
    glm::vec2 mTranslation = { 0.0f, 0.0f };

public:
	J3DTextureSRTInfo() = default;
    J3DTextureSRTInfo(const J3DTextureSRTInfo& other) = default;
    J3DTextureSRTInfo(J3DTextureSRTInfo&& other) noexcept = default;

	J3DTextureSRTInfo& operator=(J3DTextureSRTInfo const& info) {
		mScale       = info.mScale;
		mRotation    = info.mRotation;
		mTranslation = info.mTranslation;
		return *this;
	}

	inline const glm::vec2& GetScale() const { return mScale; }
    inline float GetRotation() const { return mRotation; }
    inline const glm::vec2& GetTranslation() const { return mTranslation; }

    inline void SetScale(const glm::vec2 &scale) { mScale = scale; }
    inline void SetRotation(float rotation) { mRotation = rotation; }
    inline void SetTranslation(const glm::vec2& translation) { mTranslation = translation; }

    inline void SetSRT(const glm::vec2& s, float r, const glm::vec2& t) {
        mTranslation = t;
        mRotation = r;
        mScale = s;
    }

	inline const glm::mat4& ToMat4() const { return glm::identity<glm::mat4>(); }

	void Serialize(bStream::CStream* stream);
	void Deserialize(bStream::CStream* stream);

	bool operator==(const J3DTextureSRTInfo& other) const;
	bool operator!=(const J3DTextureSRTInfo& other) const;
};
