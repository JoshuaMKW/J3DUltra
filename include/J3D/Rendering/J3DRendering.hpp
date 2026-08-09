#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <functional>
#include <memory>

#include "J3D/Util/J3DUtil.hpp"

struct J3DRenderPacket;
class J3DModelInstance;

namespace J3D {
    namespace Rendering {
        void SetSortFunction(std::function<void(std::vector<J3DRenderPacket>&)> sortFunction);
        std::vector<J3DRenderPacket> SortPackets(const shared_vector<J3DModelInstance>& modelInstances, const glm::vec3& cameraPosition);

        std::vector<J3DRenderPacket> Update(float deltaTime, glm::mat4& viewMatrix, glm::mat4& projMatrix, shared_vector<J3DModelInstance>& modelInstances, bool updateAnimations = true);
        void Render(std::vector<J3DRenderPacket>& renderPackets, uint32_t materialShaderOverride = 0);
    }
}
