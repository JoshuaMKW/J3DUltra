#include "J3D/Animation/J3DHermiteAnimationTrack.hpp"
#include "J3D/Animation/J3DAnimationKey.hpp"

#include "bstream.h"

#include <glm/glm.hpp>

//const glm::mat4 HERMITE_MTX(
//    2.0f, -2.0f, 1.0f, 1.0f,
//    -3.0f, 3.0f, -2.0f, -1.0f,
//    0.0f, 0.0f, 1.0f, 0.0f,
//    1.0f, 0.0f, 0.0f, 0.0f
//);

J3DAnimation::J3DHermiteAnimationTrack::J3DHermiteAnimationTrack() {

}

J3DAnimation::J3DHermiteAnimationTrack::~J3DHermiteAnimationTrack() {

}

float J3DAnimation::J3DHermiteAnimationTrack::GetValue(float time) const {
    if (mKeys.size() <= 1) {
        return mKeys.empty() ? 0.0f : mKeys[0].Value;
    }

    auto it = std::upper_bound(mKeys.begin(), mKeys.end(), time,
        [](float t, const J3DAnimationKey& key) {
            return t < key.Time;
        });

    const size_t index = std::distance(mKeys.begin(), it);

    if (index == mKeys.size()) {
        return mKeys.back().Value;
    }

    if (index == 0) {
        return mKeys.front().Value;
    }

    const J3DAnimationKey* firstKey = &mKeys[index - 1];
    const J3DAnimationKey* secondKey = &mKeys[index];

    float frameTime = (time - firstKey->Time) / (secondKey->Time - firstKey->Time);
    return InterpolateValue(frameTime, firstKey, secondKey);
}

float J3DAnimation::J3DHermiteAnimationTrack::InterpolateValue(float time, const J3DAnimationKey* a, const J3DAnimationKey* b) const {
    const float framesBetweenKeys = b->Time - a->Time;

    const float t2 = time * time;
    const float t3 = t2 * time;

    const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    const float h01 = -2.0f * t3 + 3.0f * t2;
    const float h10 = t3 - 2.0f * t2 + time;
    const float h11 = t3 - t2;

    const float p0 = a->Value;
    const float p1 = b->Value;
    const float m0 = a->OutTangent * framesBetweenKeys;
    const float m1 = b->InTangent * framesBetweenKeys;

    return (h00 * p0) + (h01 * p1) + (h10 * m0) + (h11 * m1);
}

void J3DAnimation::J3DHermiteAnimationTrack::ReserveKeys(size_t capacity)
{
    mKeys.reserve(capacity);
}

void J3DAnimation::J3DHermiteAnimationTrack::AddKey(J3DAnimation::J3DAnimationKey key)
{
    mKeys.push_back(key);
}
