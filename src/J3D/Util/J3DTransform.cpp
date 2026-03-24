#include "J3D/Util/J3DTransform.hpp"

#include <bstream.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

float U16ToFloat(int16_t val) {
	return val * (180 / 32768.0f);
}

uint16_t FloatToU16(float val) {
	return (uint16_t)(val * (32768.f / 180.f));
}

/* == J3DTransformInfo == */

void J3DTransformInfo::CalculateSRTMatrix() const {
    mSRTMatrix = glm::translate(mTranslation) * glm::toMat4(mRotation) * glm::scale(mScale);
}

void J3DTransformInfo::Deserialize(bStream::CStream* stream) {
	mScale.x = stream->readFloat();
	mScale.y = stream->readFloat();
	mScale.z = stream->readFloat();

	glm::vec3 eulerRotation;
	eulerRotation.x = glm::radians(U16ToFloat(stream->readInt16()));
	eulerRotation.y = glm::radians(U16ToFloat(stream->readInt16()));
	eulerRotation.z = glm::radians(U16ToFloat(stream->readInt16()));

	mRotation = glm::quat(eulerRotation);

	stream->skip(2);

	mTranslation.x = stream->readFloat();
	mTranslation.y = stream->readFloat();
	mTranslation.z = stream->readFloat();

	CalculateSRTMatrix();
}

bool J3DTransformInfo::operator==(const J3DTransformInfo& other) const {
	return mScale == other.mScale && mRotation == other.mRotation && mTranslation == other.mTranslation;
}

bool J3DTransformInfo::operator!=(const J3DTransformInfo& other) const {
	return !operator==(other);
}

/* == J3DTextureSRTInfo == */
void J3DTextureSRTInfo::Serialize(bStream::CStream* stream) {
	stream->writeFloat(mScale.x);
	stream->writeFloat(mScale.y);

	stream->writeUInt16(FloatToU16(glm::degrees(mRotation)));
	stream->writeUInt16(UINT16_MAX);

	stream->writeFloat(mTranslation.x);
	stream->writeFloat(mTranslation.y);
}

void J3DTextureSRTInfo::Deserialize(bStream::CStream* stream) {
	mScale.x = stream->readFloat();
	mScale.y = stream->readFloat();

	mRotation = glm::radians(U16ToFloat(stream->readInt16()));

	stream->skip(2);

	mTranslation.x = stream->readFloat();
	mTranslation.y = stream->readFloat();
}

bool J3DTextureSRTInfo::operator==(const J3DTextureSRTInfo& other) const {
	return mScale == other.mScale && mRotation == other.mRotation && mTranslation == other.mTranslation;
}

bool J3DTextureSRTInfo::operator!=(const J3DTextureSRTInfo& other) const {
	return !operator==(other);
}
