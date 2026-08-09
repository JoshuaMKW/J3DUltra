#include <algorithm>
#include <execution>
#include <vector>

#include "J3D/Rendering/J3DRendering.hpp"
#include "J3D/Rendering/J3DRenderPacket.hpp"
#include "J3D/Data/J3DModelInstance.hpp"

namespace J3D {
    namespace Rendering {
        namespace {
            std::function<void(RenderPacketVector&)> SortFunction = [](RenderPacketVector) {};
        }
    }
}

void J3D::Rendering::SetSortFunction(std::function<void(RenderPacketVector&)> sortFunction) {
    if (sortFunction) {
        SortFunction = sortFunction;
    }
}

J3D::Rendering::RenderPacketVector J3D::Rendering::SortPackets(ModelInstanceVector& modelInstances, glm::vec3 cameraPosition) {
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

void J3D::Rendering::Update(float deltaTime, glm::mat4& viewMatrix, glm::mat4& projMatrix, RenderPacketVector& renderPackets, bool updateAnimations) {
    if (updateAnimations) {
        // Collect the unique model instances from the render packets to avoid updating the same instance animations multiple times.
        std::vector<J3DModelInstance*> instances;
        instances.reserve(renderPackets.size());
        for (const J3DRenderPacket& packet : renderPackets) {
            if (packet.Instance != nullptr) {
                instances.push_back(packet.Instance);
            }
        }

        // Sort and remove duplicates
        std::sort(instances.begin(), instances.end());
        instances.erase(std::unique(instances.begin(), instances.end()), instances.end());

        std::for_each(std::execution::par, instances.begin(), instances.end(), [deltaTime](J3DModelInstance* instance) {
            instance->UpdateAnimations(deltaTime);
        });
    }

    std::for_each(std::execution::par, renderPackets.begin(), renderPackets.end(), [&deltaTime, &viewMatrix, &projMatrix, updateAnimations](J3DRenderPacket& packet) {
        packet.Update(deltaTime, viewMatrix, projMatrix, updateAnimations);
    });
}

void J3D::Rendering::Render(RenderPacketVector& renderPackets, uint32_t materialShaderOverride) {
    for (J3DRenderPacket &packet : renderPackets) {
        packet.Render(materialShaderOverride);
    }
}
