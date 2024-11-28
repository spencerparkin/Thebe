# CMakeLists.txt for the Thebe engine library.

set(GRAPHICS_ENGINE_SOURCES
    Source/Thebe/Application.cpp
    Source/Thebe/Application.h
    Source/Thebe/GraphicsEngine.cpp
    Source/Thebe/GraphicsEngine.h
    Source/Thebe/Common.h
    Source/Thebe/Reference.cpp
    Source/Thebe/Reference.h
    Source/Thebe/Log.cpp
    Source/Thebe/Log.h
    Source/Thebe/Utilities/Clock.cpp
    Source/Thebe/Utilities/Clock.h
    Source/Thebe/Utilities/BlockManager.cpp
    Source/Thebe/Utilities/BlockManager.h
    Source/Thebe/Containers/AVLTree.cpp
    Source/Thebe/Containers/AVLTree.h
    Source/Thebe/Containers/LinkedList.cpp
    Source/Thebe/Containers/LinkedList.h
    Source/Thebe/EnginePart.cpp
    Source/Thebe/EnginePart.h
    Source/Thebe/EngineParts/Camera.cpp
    Source/Thebe/EngineParts/Camera.h
    Source/Thebe/EngineParts/Material.cpp
    Source/Thebe/EngineParts/Material.h
    Source/Thebe/EngineParts/Mesh.cpp
    Source/Thebe/EngineParts/Mesh.h
    Source/Thebe/EngineParts/RenderObject.cpp
    Source/Thebe/EngineParts/RenderObject.h
    Source/Thebe/EngineParts/RenderPass.cpp
    Source/Thebe/EngineParts/RenderPass.h
    Source/Thebe/EngineParts/RenderTarget.cpp
    Source/Thebe/EngineParts/RenderTarget.h
    Source/Thebe/EngineParts/MainRenderPass.cpp
    Source/Thebe/EngineParts/MainRenderPass.h
    Source/Thebe/EngineParts/Space.cpp
    Source/Thebe/EngineParts/Space.h
    Source/Thebe/EngineParts/SwapChain.cpp
    Source/Thebe/EngineParts/SwapChain.h
    Source/Thebe/EngineParts/Shader.cpp
    Source/Thebe/EngineParts/Shader.h
    Source/Thebe/EngineParts/Texture.cpp
    Source/Thebe/EngineParts/Texture.h
    Source/Thebe/EngineParts/Buffer.cpp
    Source/Thebe/EngineParts/Buffer.h
    Source/Thebe/EngineParts/VertexBuffer.cpp
    Source/Thebe/EngineParts/VertexBuffer.h
    Source/Thebe/EngineParts/IndexBuffer.cpp
    Source/Thebe/EngineParts/IndexBuffer.h
    Source/Thebe/EngineParts/ConstantsBuffer.cpp
    Source/Thebe/EngineParts/ConstantsBuffer.h
    Source/Thebe/EngineParts/Fence.cpp
    Source/Thebe/EngineParts/Fence.h
    Source/Thebe/EngineParts/Scene.cpp
    Source/Thebe/EngineParts/Scene.h
    Source/Thebe/EngineParts/CommandQueue.cpp
    Source/Thebe/EngineParts/CommandQueue.h
    Source/Thebe/EngineParts/CommandExecutor.cpp
    Source/Thebe/EngineParts/CommandExecutor.h
    Source/Thebe/EngineParts/MeshInstance.cpp
    Source/Thebe/EngineParts/MeshInstance.h
    Source/Thebe/EngineParts/DescriptorHeap.cpp
    Source/Thebe/EngineParts/DescriptorHeap.h
    Source/Thebe/EngineParts/UploadHeap.cpp
    Source/Thebe/EngineParts/UploadHeap.h
    Source/Thebe/Math/AnimTransform.cpp
    Source/Thebe/Math/AnimTransform.h
    Source/Thebe/Math/AxisAlignedBoundingBox.cpp
    Source/Thebe/Math/AxisAlignedBoundingBox.h
    Source/Thebe/Math/Frustum.cpp
    Source/Thebe/Math/Frustum.h
    Source/Thebe/Math/Matrix4x4.cpp
    Source/Thebe/Math/Matrix4x4.h
    Source/Thebe/Math/Matrix3x3.cpp
    Source/Thebe/Math/Matrix3x3.h
    Source/Thebe/Math/Matrix2x2.cpp
    Source/Thebe/Math/Matrix2x2.h
    Source/Thebe/Math/Transform.cpp
    Source/Thebe/Math/Transform.h
    Source/Thebe/Math/Vector4.cpp
    Source/Thebe/Math/Vector4.h
    Source/Thebe/Math/Vector3.cpp
    Source/Thebe/Math/Vector3.h
    Source/Thebe/Math/Vector2.cpp
    Source/Thebe/Math/Vector2.h
    Source/Thebe/Math/Ray.cpp
    Source/Thebe/Math/Ray.h
    Source/Thebe/Math/Plane.cpp
    Source/Thebe/Math/Plane.h
    Source/Thebe/Math/LineSegment.cpp
    Source/Thebe/Math/LineSegment.h
    Source/Thebe/Math/Quaternion.cpp
    Source/Thebe/Math/Quaternion.h
    Source/Thebe/Math/Function.cpp
    Source/Thebe/Math/Function.h
    Source/Thebe/Math/Interval.cpp
    Source/Thebe/Math/Interval.h
    Source/Thebe/Math/SphericalCoords.cpp
    Source/Thebe/Math/SphericalCoords.h
    Source/Thebe/Math/Angle.cpp
    Source/Thebe/Math/Angle.h
    Source/Thebe/Math/Polygon.cpp
    Source/Thebe/Math/Polygon.h
    Source/Thebe/Math/PolygonMesh.cpp
    Source/Thebe/Math/PolygonMesh.h
    Source/Thebe/Math/Graph.cpp
    Source/Thebe/Math/Graph.h
    Source/Thebe/Math/Random.cpp
    Source/Thebe/Math/Random.h
)

source_group("Source" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GAME_ENGINE_SOURCES})

add_library(ThebeGraphicsEngine SHARED
    ${GRAPHICS_ENGINE_SOURCES}
)

target_compile_definitions(ThebeGraphicsEngine PRIVATE
    THEBE_EXPORT
    _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(ThebeGraphicsEngine PUBLIC
        THEBE_LOGGING
        THEBE_LOG_FRAMERATE
    )
endif()

target_link_libraries(ThebeGraphicsEngine PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    ParseParty
)

target_include_directories(ThebeGraphicsEngine PUBLIC
    "Source"
    "DirectXHeader"
    "../ThirdParty/ParseParty/ParseLibrary/Source"
)