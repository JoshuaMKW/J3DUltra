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

void J3DSkeleton::CalculateAnimJointPose(const std::vector<glm::mat4>& transforms, std::vector<glm::mat4>& skinningMatrices, std::vector<glm::mat4>& out) {
    if (out.size() != mEnvelopeIndices.size()) {
        return;
    }

    const size_t limit = std::min(transforms.size(), mInverseBindMatrices.size());
    for (size_t i = 0; i < limit; ++i) {
        skinningMatrices[i] = transforms[i] * mInverseBindMatrices[i];
    }

    for (int i = 0; i < mEnvelopeIndices.size(); i++) {
        if (mDrawBools[i] == false) {
            out[i] = transforms[mEnvelopeIndices[i]];
        } else {
            glm::mat4 matrix = glm::zero<glm::mat4>();

            const J3DEnvelope& env = mJointEnvelopes[mEnvelopeIndices[i]];

            for (int j = 0; j < env.Weights.size(); j++) {
                uint32_t jointIndex = env.JointIndices[j];

                matrix += skinningMatrices[jointIndex] * env.Weights[j];
            }

            out[i] = matrix;
        }
    }
}
