#include "J3D/Data/J3DModelInstance.hpp"
#include "J3D/Data/J3DModelData.hpp"
#include "J3D/Material/J3DUniformBufferObject.hpp"
#include "J3D/Material/J3DMaterialTable.hpp"

#include "J3D/Animation/J3DColorAnimationInstance.hpp"
#include "J3D/Animation/J3DTexIndexAnimationInstance.hpp"
#include "J3D/Animation/J3DTexMatrixAnimationInstance.hpp"
#include "J3D/Animation/J3DJointAnimationInstance.hpp"
#include "J3D/Animation/J3DJointFullAnimationInstance.hpp"
#include "J3D/Animation/J3DVisibilityAnimationInstance.hpp"

#include "J3D/Skeleton/J3DJoint.hpp"

#include <stdexcept>
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

J3DModelInstance::J3DModelInstance(std::shared_ptr<J3DModelData> modelData, uint16_t id) {
	if (modelData == nullptr)
		throw std::invalid_argument("Tried to create a J3DModelInstance from invalid J3DModelData pointer!");

	mModelData = modelData;
	mEnvelopeMatrices = mModelData->GetRestPose();
	mReferenceFrame = glm::identity<glm::mat4>();
    mModelMatrix = glm::identity<glm::mat4>();
	mSortBias = 0;
	mModelId = id;
	bUseInstanceMaterialTable = false;

	mBBMin = { 0, 0, 0 };
    mBBMax = { 0, 0, 0 };
}

J3DModelInstance::~J3DModelInstance() {

}

void J3DModelInstance::CalculateJointMatrices(float deltaTime) {
	if (mJointAnimation == nullptr && mJointFullAnimation == nullptr) {
		return;
	}

	if (mJointAnimation != nullptr) {
		mJointAnimation->GetTransformsAtFrame(deltaTime, mAnimationMatrices);
	}
	else if (mJointFullAnimation != nullptr) {
		mJointFullAnimation->GetTransformsAtFrame(deltaTime, mAnimationMatrices);
	}
	else {
		mAnimationMatrices.assign(mModelData->GetJoints().size(), glm::identity<glm::mat4>());
	}
        
	std::vector<glm::mat4> t(mModelData->GetJoints().size());

    for (size_t i = 0; i < mModelData->GetJoints().size(); ++i) {
        const std::shared_ptr<J3DJoint>& jnt = mModelData->GetJoints()[i];
        glm::mat4 localTransform = mAnimationMatrices[jnt->GetJointID()];

		if (jnt->GetAttachFlag() == 1) {
            // Strip scale by normalizing the basis vectors (Columns 0, 1, 2)
            localTransform[0] = glm::vec4(glm::normalize(glm::vec3(localTransform[0])), 0.0f);
            localTransform[1] = glm::vec4(glm::normalize(glm::vec3(localTransform[1])), 0.0f);
            localTransform[2] = glm::vec4(glm::normalize(glm::vec3(localTransform[2])), 0.0f);
		}
		
		std::shared_ptr<J3DJoint> parent = std::static_pointer_cast<J3DJoint>(jnt->GetParent().lock());
		if (parent != nullptr) {
			// Multiply local transform by the parent's ALREADY CALCULATED global transform
			// (Assuming parent is guaranteed to have a lower index than the child)
			uint32_t parentID = std::static_pointer_cast<J3DJoint>(parent)->GetJointID();
			t[i] = t[parentID] * localTransform; 
		} else {
			// It's the root bone
			t[i] = localTransform;
		}
	}

	mModelData->CalculateAnimJointPose(t, mEnvelopeMatrices);
}

void J3DModelInstance::UpdateMaterialTextureMatrices(float deltaTime, std::shared_ptr<J3DMaterial> material, glm::mat4& viewMatrix, glm::mat4& projMatrix) {
	if (mTexMatrixAnimation != nullptr) {
		mTexMatrixAnimation->ApplyAnimation(material);
	}

	material->CalculateTexMatrices(mTransform.ToMat4(), viewMatrix, projMatrix);
}

void J3DModelInstance::UpdateMaterialTextures(float deltaTime, std::shared_ptr<J3DMaterial> material) {
	if (mTexIndexAnimation == nullptr) {
		return;
	}

	mTexIndexAnimation->ApplyAnimation(material);
}

void J3DModelInstance::UpdateMaterialColors(float deltaTime) {
	// TODO: implement BPK
}

void J3DModelInstance::UpdateTEVRegisterColors(float deltaTime, std::shared_ptr<J3DMaterial> material) {
	if (mRegisterColorAnimation == nullptr) {
		return;
	}

	mRegisterColorAnimation->ApplyAnimation(material);
}

void J3DModelInstance::UpdateShapeVisibility(float deltaTime) {
	if (mVisibilityAnimation == nullptr) {
		return;
	}

	shared_vector<GXShape>& shapes = mModelData->GetShapes();
	for (uint32_t i = 0; i < shapes.size(); i++) {
		shapes[i]->SetVisible(mVisibilityAnimation->GetVisibilityAtFrame(i, deltaTime));
	}
}

void J3DModelInstance::Update(float deltaTime, std::shared_ptr<J3DMaterial> material, glm::mat4& viewMatrix, glm::mat4& projMatrix, bool updateAnimations) {
    if (!updateAnimations) {
        return;
	}

    UpdateAnimations(deltaTime);
    UpdateTEVRegisterColors(deltaTime, material);
    UpdateMaterialTextures(deltaTime, material);
    UpdateMaterialTextureMatrices(deltaTime, material, viewMatrix, projMatrix);
    UpdateShapeVisibility(deltaTime);
    CalculateJointMatrices(deltaTime);
}

void J3DModelInstance::SetTranslation(const glm::vec3 &trans) {
	mTransform.SetTranslation(trans);
    mModelMatrix = mReferenceFrame * mTransform.ToMat4();
}

void J3DModelInstance::SetRotation(const glm::vec3 &rot) {
    glm::vec3 eulerRotation = glm::radians(rot);
    mTransform.SetRotation(glm::quat(eulerRotation));
    mModelMatrix = mReferenceFrame * mTransform.ToMat4();
}

void J3DModelInstance::SetScale(const glm::vec3 &scale) {
    mTransform.SetScale(scale);
    mModelMatrix = mReferenceFrame * mTransform.ToMat4();
}

void J3DModelInstance::SetTransform(const glm::mat4 &transform) {
	glm::vec3 translation, scale, skew;
	glm::vec4 perspective;
	glm::quat rotation;

	glm::decompose(transform, scale, rotation, translation, skew, perspective);

	mTransform.SetSRT(scale, rotation, translation);
    mModelMatrix = mReferenceFrame * mTransform.ToMat4();
}

void J3DModelInstance::GetBoundingBox(glm::vec3& min, glm::vec3& max) const {
	mModelData->GetBoundingBox(min, max);
}

const shared_vector<J3DMaterial>& J3DModelInstance::GetMaterials() const {
	return CheckUseInstanceMaterials() ? mInstanceMaterialTable->GetMaterials() : mModelData->GetMaterials();
}

J3DLight J3DModelInstance::GetLight(int index) const {
	J3DLight light;

	if (index >= 0 && index < 8) {
		light = mLights[index];
	}

	return light;
}

void J3DModelInstance::SetLight(const J3DLight& light, int index) {
	if (index < 0 || index >= 8) {
		return;
	}

	mLights[index] = light;
}

void J3DModelInstance::SetReferenceFrame(const glm::mat4 &frame) {
    mReferenceFrame = frame;
    mModelMatrix = mReferenceFrame * mTransform.ToMat4();
}

void J3DModelInstance::GatherRenderPackets(std::vector<J3DRenderPacket>& packetList, glm::vec3 cameraPosition) {
	const shared_vector<J3DMaterial>& materials = CheckUseInstanceMaterials() ? mInstanceMaterialTable->GetMaterials() : mModelData->GetMaterials();

    packetList.reserve(packetList.size() + materials.size() + 1);

    for (const std::shared_ptr<J3DMaterial> &mat : materials)
    {
		std::shared_ptr<GXShape> lockedShape = mat->GetShape().lock();
        if (!lockedShape) {
            continue;
		}

		const glm::vec3& center = lockedShape->GetCenterOfMass();
		glm::vec4 transformedCenter = mModelMatrix * glm::vec4(center, 1.0f);

		float distToCamera = glm::distance(cameraPosition, glm::vec3(transformedCenter));
		uint32_t sortKey = static_cast<uint32_t>(distToCamera) & 0x7FFFFF;

		const bool isOpaqueOrAlpha =
            (mat->PEMode == EPixelEngineMode::Opaque || mat->PEMode == EPixelEngineMode::AlphaTest);

        sortKey |= (isOpaqueOrAlpha ? 0x00800000 : 0);
		sortKey |= mSortBias << 24;

        packetList.emplace_back(J3DRenderPacket { sortKey, mat, this });
	}
}

void J3DModelInstance::UpdateAnimations(float deltaTime) {
	if (mRegisterColorAnimation != nullptr) {
		mRegisterColorAnimation->Tick(deltaTime);
	}

	if (mTexIndexAnimation != nullptr) {
		mTexIndexAnimation->Tick(deltaTime);
	}

	if (mTexMatrixAnimation != nullptr) {
		mTexMatrixAnimation->Tick(deltaTime);
	}

	if (mJointAnimation != nullptr) {
		mJointAnimation->Tick(deltaTime);
	}

	if (mJointAnimation == nullptr && mJointFullAnimation != nullptr) {
		mJointFullAnimation->Tick(deltaTime);
	}

	if (mVisibilityAnimation != nullptr) {
		mVisibilityAnimation->Tick(deltaTime);
	}
}

void J3DModelInstance::Render(const std::shared_ptr<J3DMaterial> &material, uint32_t materialShaderOverride)
{
	J3DUniformBufferObject::SetEnvelopeMatrices(mEnvelopeMatrices.data(), (uint32_t)mEnvelopeMatrices.size());
	J3DUniformBufferObject::SetLights(mLights);
	J3DUniformBufferObject::SetModelMatrix(mModelMatrix);
	J3DUniformBufferObject::SetModelId(mModelId);

	mModelData->BindVAO();

	auto& textures = CheckUseInstanceTextures() ? mInstanceMaterialTable->GetTextures() : mModelData->GetTextures();
	material->Render(textures, materialShaderOverride);

	mModelData->UnbindVAO();
}

bool J3DModelInstance::CheckUseInstanceMaterials() const {
	return bUseInstanceMaterialTable && mInstanceMaterialTable != nullptr && mInstanceMaterialTable->GetMaterials().size() != 0;
}

bool J3DModelInstance::CheckUseInstanceTextures() const {
	return bUseInstanceMaterialTable && mInstanceMaterialTable != nullptr && mInstanceMaterialTable->GetTextures().size() != 0;
}

void J3DModelInstance::SetJointAnimation(std::shared_ptr<J3DAnimation::J3DJointAnimationInstance> anim) {
    if (anim != nullptr && anim->GetJointCount() != mModelData->GetJointCount()) {
        return;
    }

	mJointAnimation = anim;
}

void J3DModelInstance::SetJointFullAnimation(std::shared_ptr<J3DAnimation::J3DJointFullAnimationInstance> anim) {
    if (anim != nullptr && anim->GetJointCount() != mModelData->GetJointCount()) {
        return;
    }

	mJointFullAnimation = anim;
}
