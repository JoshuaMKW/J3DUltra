#include <algorithm>
#include <execution>
#include <vector>

#include "J3D/Rendering/J3DRendering.hpp"
#include "J3D/Rendering/J3DRenderPacket.hpp"
#include "J3D/Data/J3DModelInstance.hpp"
#include "J3D/Util/J3DUtil.hpp"

namespace J3D {
    namespace Rendering {
        namespace {
            std::function<void(std::vector<J3DRenderPacket>&)> SortFunction = [](std::vector<J3DRenderPacket> &) { };
        }

        void SetSortFunction(std::function<void(std::vector<J3DRenderPacket>&)> sortFunction)
        {
            if (sortFunction) {
                SortFunction = sortFunction;
            }
        }
    }
}

std::vector<J3DRenderPacket> J3D::Rendering::SortPackets(const shared_vector<J3DModelInstance>& modelInstances, const glm::vec3& cameraPosition)
{
    std::vector<J3DRenderPacket> packets;

	size_t materialCount = 0;
    for (std::shared_ptr<J3DModelInstance> instance : modelInstances) {
        materialCount += instance->GetMaterials().size();
    }

	packets.reserve(materialCount);

    for (std::shared_ptr<J3DModelInstance> instance : modelInstances) {
        instance->GatherRenderPackets(packets, cameraPosition);
    }

    SortFunction(packets);
    return packets;
}

std::vector<J3DRenderPacket> J3D::Rendering::Update(float deltaTime, glm::mat4& viewMatrix, glm::mat4& projMatrix, shared_vector<J3DModelInstance>& modelInstances, bool updateAnimations) {
    if (updateAnimations) {
        std::for_each(std::execution::par, modelInstances.begin(), modelInstances.end(), [deltaTime](std::shared_ptr<J3DModelInstance> instance) {
            instance->UpdateAnimations(deltaTime);
        });
    }

    const glm::vec3 position = glm::vec3(glm::inverse(viewMatrix)[3]);
    std::vector<J3DRenderPacket> packets = J3D::Rendering::SortPackets(modelInstances, position);

    std::for_each(std::execution::par, packets.begin(), packets.end(), [&deltaTime, &viewMatrix, &projMatrix, updateAnimations](J3DRenderPacket& packet) {
        packet.Update(deltaTime, viewMatrix, projMatrix, updateAnimations);
    });

    return packets;
}

void J3D::Rendering::Render(std::vector<J3DRenderPacket>& renderPackets, uint32_t materialShaderOverride)
{
    for (J3DRenderPacket &packet : renderPackets) {
        packet.Render(materialShaderOverride);
    }
}
