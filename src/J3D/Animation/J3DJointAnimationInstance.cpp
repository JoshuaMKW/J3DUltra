#include "J3D/Animation/J3DJointAnimationInstance.hpp"

#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>


J3DAnimation::J3DJointAnimationInstance::J3DJointAnimationInstance() {

}

void J3DAnimation::J3DJointAnimationInstance::ReadFloatComponentTrack(bStream::CStream& stream, J3DHermiteAnimationTrack& track, uint32_t valueTableOffset) {
    uint16_t keyCount = stream.readUInt16();
    uint16_t firstKeyIndex = stream.readUInt16();
    ETangentMode tangentMode = static_cast<ETangentMode>(stream.readUInt16());

    size_t currentStreamPos = stream.tell();
    stream.seek(valueTableOffset + firstKeyIndex * sizeof(float));

    track.ReserveKeys(keyCount);

    if (keyCount == 1) {
        J3DAnimationKey newKey;
        newKey.Value = stream.readFloat();

        track.AddKey(newKey);
    }
    else {
        for (uint16_t i = 0; i < keyCount; i++) {
            J3DAnimationKey newKey;

            switch (tangentMode) {
            case ETangentMode::Symmetric:
                newKey.Time = stream.readFloat();
                newKey.Value = stream.readFloat();
                newKey.InTangent = stream.readFloat();
                newKey.OutTangent = newKey.InTangent;
                break;
            case ETangentMode::Piecewise:
                newKey.Time = stream.readFloat();
                newKey.Value = stream.readFloat();
                newKey.InTangent = stream.readFloat();
                newKey.OutTangent = stream.readFloat();
                break;
            default:
                break;
            }

            track.AddKey(newKey);
        }
    }

    stream.seek(currentStreamPos);
}

void J3DAnimation::J3DJointAnimationInstance::ReadRotationComponentTrack(
    bStream::CStream& stream,
    J3DHermiteAnimationTrack& track,
    uint32_t valueTableOffset, float scale
) {
    uint16_t keyCount = stream.readUInt16();
    uint16_t firstKeyIndex = stream.readUInt16();
    ETangentMode tangentMode = static_cast<ETangentMode>(stream.readUInt16());

    size_t currentStreamPos = stream.tell();
    stream.seek(valueTableOffset + firstKeyIndex * sizeof(uint16_t));

    track.ReserveKeys(keyCount);

    if (keyCount == 1) {
        J3DAnimationKey newKey;
        newKey.Value = static_cast<float>(stream.readInt16()) * scale;

        track.AddKey(newKey);
    }
    else {
        for (uint16_t i = 0; i < keyCount; i++) {
            J3DAnimationKey newKey;

            switch (tangentMode) {
            case ETangentMode::Symmetric:
                newKey.Time = static_cast<float>(stream.readInt16());
                newKey.Value = static_cast<float>(stream.readInt16()) * scale;
                newKey.InTangent = static_cast<float>(stream.readInt16()) * scale;
                newKey.OutTangent = newKey.InTangent;
                break;
            case ETangentMode::Piecewise:
                newKey.Time = static_cast<float>(stream.readInt16());
                newKey.Value = static_cast<float>(stream.readInt16()) * scale;
                newKey.InTangent = static_cast<float>(stream.readInt16()) * scale;
                newKey.OutTangent = static_cast<float>(stream.readInt16()) * scale;
                break;
            default:
                break;
            }

            track.AddKey(newKey);
        }
    }

    stream.seek(currentStreamPos);
}

void J3DAnimation::J3DJointAnimationInstance::Deserialize(bStream::CStream& stream) {
    size_t currentStreamPos = stream.tell();

    // Deserialize counts and offsets from the block
    J3DJointKeyBlock jointKeyBlock;
    jointKeyBlock.Deserialize(&stream);

    mLength = jointKeyBlock.Length;
    mLoopMode = static_cast<ELoopMode>(jointKeyBlock.LoopMode);

    size_t fullTrackCount = jointKeyBlock.TrackCount;

    float rotationScale = (1 << jointKeyBlock.RotationFraction) * (180.0f / 32768.0f);

    mEntries.reserve(fullTrackCount);

    stream.seek(jointKeyBlock.TrackTableOffset);
    for (int i = 0; i < fullTrackCount; i++) {
        J3DJointAnimationData animData;
        animData.JointIndex = i;

        ReadFloatComponentTrack(stream, animData.ScaleX, jointKeyBlock.ScaleTableOffset);
        ReadRotationComponentTrack(stream, animData.RotationX, jointKeyBlock.RotationTableOffset, rotationScale);
        ReadFloatComponentTrack(stream, animData.TranslationX, jointKeyBlock.TranslationTableOffset);

        ReadFloatComponentTrack(stream, animData.ScaleY, jointKeyBlock.ScaleTableOffset);
        ReadRotationComponentTrack(stream, animData.RotationY, jointKeyBlock.RotationTableOffset, rotationScale);
        ReadFloatComponentTrack(stream, animData.TranslationY, jointKeyBlock.TranslationTableOffset);

        ReadFloatComponentTrack(stream, animData.ScaleZ, jointKeyBlock.ScaleTableOffset);
        ReadRotationComponentTrack(stream, animData.RotationZ, jointKeyBlock.RotationTableOffset, rotationScale);
        ReadFloatComponentTrack(stream, animData.TranslationZ, jointKeyBlock.TranslationTableOffset);

        mEntries.push_back(animData);
    }

    stream.seek(currentStreamPos + jointKeyBlock.BlockSize);
}

void J3DAnimation::J3DJointAnimationInstance::GetTransformsAtFrame(float deltaTime, std::vector<glm::mat4> &out) {
    float frameTime = GetFrame();

    if (out.size() != mEntries.size()) {
        out.resize(mEntries.size());
    }

    size_t i = 0;
    for (const J3DJointAnimationData& j : mEntries) {
        glm::vec3 translation = glm::vec3(j.TranslationX.GetValue(frameTime), j.TranslationY.GetValue(frameTime), j.TranslationZ.GetValue(frameTime));
        glm::vec3 scale = glm::vec3(j.ScaleX.GetValue(frameTime), j.ScaleY.GetValue(frameTime), j.ScaleZ.GetValue(frameTime));
        glm::vec3 eulerRotation = glm::vec3(
            glm::radians(j.RotationX.GetValue(frameTime)),
            glm::radians(j.RotationY.GetValue(frameTime)),
            glm::radians(j.RotationZ.GetValue(frameTime))
        );

#if 0
        glm::quat rotation = glm::angleAxis(eulerRotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) *
                             glm::angleAxis(eulerRotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::angleAxis(eulerRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));


        transforms.emplace_back(glm::translate(translation) * glm::toMat4(rotation) * glm::scale(scale));
#else
        glm::mat4 transform = glm::eulerAngleZYX(eulerRotation.z, eulerRotation.y, eulerRotation.x);

        transform[3] = glm::vec4(translation, 1.0f);

        transform[0] *= scale.x;
        transform[1] *= scale.y;
        transform[2] *= scale.z;

        out[i++] = std::move(transform);
#endif
    }
}