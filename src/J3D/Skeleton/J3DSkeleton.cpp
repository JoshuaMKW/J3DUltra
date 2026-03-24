#include "J3D/Skeleton/J3DSkeleton.hpp"
#include "J3D/Skeleton/J3DJoint.hpp"



J3DSkeleton::J3DSkeleton() {

}

std::shared_ptr<J3DJoint> J3DSkeleton::GetJoint(uint32_t index) {
    return index < mJoints.size() ? mJoints[index] : std::shared_ptr<J3DJoint>();
}

std::shared_ptr<J3DJoint> J3DSkeleton::GetJoint(std::string name) {
    for (uint32_t i = 0; i < mJoints.size(); i++) {
        if (mJoints[i]->GetJointName() == name) {
            return GetJoint(i);
        }
    }

    return std::shared_ptr<J3DJoint>();
}

void J3DSkeleton::CalculateRestPose() {
    mRestPose.clear();
    mRestPose.reserve(mEnvelopeIndices.size());
    for (int i = 0; i < mEnvelopeIndices.size(); i++) {
        if (mDrawBools[i] == false) {
            mRestPose.push_back(mJoints[mEnvelopeIndices[i]]->GetTransformMatrix());
        }
        else {
            glm::mat4 matrix = glm::zero<glm::mat4>();

            const J3DEnvelope &env = mJointEnvelopes[mEnvelopeIndices[i]];
            float weightTotal = 0.f;

            for (int j = 0; j < env.Weights.size(); j++) {
                uint32_t jointIndex = env.JointIndices[j];

                const glm::mat4 &ibm = mInverseBindMatrices[jointIndex];
                const glm::mat4 &jointTransform = mJoints[jointIndex]->GetTransformMatrix();

                matrix += (jointTransform * ibm) * env.Weights[j];
                weightTotal += env.Weights[j];
            }

            mRestPose.push_back(matrix);
        }
    }
}

std::vector<glm::mat4> J3DSkeleton::CalculateAnimJointPose(const std::vector<glm::mat4>& transforms) {
    if (mSkinningMatricesCache.size() < transforms.size()) {
        mSkinningMatricesCache.resize(transforms.size());
    }

    if (mAnimTransformsCache.size() < mEnvelopeIndices.size()) {
        mAnimTransformsCache.resize(mEnvelopeIndices.size());
    }

    const size_t limit = std::min(transforms.size(), mInverseBindMatrices.size());
    for (size_t i = 0; i < mSkinningMatricesCache.size(); ++i) {
        mSkinningMatricesCache[i] = transforms[i] * mInverseBindMatrices[i];
    }

    for (int i = 0; i < mEnvelopeIndices.size(); i++) {
        if (mDrawBools[i] == false) {
            mAnimTransformsCache[i] = transforms[mEnvelopeIndices[i]];
        }
        else {
            glm::mat4 matrix = glm::zero<glm::mat4>();

            const J3DEnvelope &env = mJointEnvelopes[mEnvelopeIndices[i]];

            for (int j = 0; j < env.Weights.size(); j++) {
                uint32_t jointIndex = env.JointIndices[j];

                matrix += mSkinningMatricesCache[jointIndex] * env.Weights[j];
            }

            mAnimTransformsCache[i] = matrix;
        }
    }

    return mAnimTransformsCache;
}
