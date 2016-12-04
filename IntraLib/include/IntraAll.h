﻿#pragma once

#include "Algorithms/Algorithms.h"
#include "Algorithms/Hash.h"
#include "Algorithms/Operations.h"
#include "Range/Concepts.h"
#include "Range/Mixins/RangeMixins.h"
#include "Range/ArrayRange.h"
#include "Range/Construction/Construction.h"
#include "Range/Iteration/Iteration.h"
#include "Algorithms/Sort.h"
#include "Algorithms/AsciiString.h"
#include "Range/SparseRange.h"
#include "Range/AssociativeRange.h"
#include "Range/Polymorphic.h"

#include "CompilerSpecific/CompilerSpecific.h"
#include "Containers/Array.h"
#include "Containers/Array2D.h"
#include "Containers/List.h"
#include "Range/StringView.h"
#include "Containers/String.h"
#include "Containers/LinearMap.h"
#include "Containers/HashMap.h"
#include "Containers/Tree.h"
#include "Containers/RangeAllocator.h"
#include "Containers/Set.h"
#include "Containers/SparseArray.h"
#include "Containers/SparseHandledArray.h"
#include "Containers/AsciiSet.h"
#include "Containers/StructureOfArrays.h"
#include "Containers/IdAllocator.h"

#include "Core/Core.h"
#include "Core/Debug.h"
#include "Core/Errors.h"
#include "Core/FundamentalTypes.h"
#include "Core/MiscTypes.h"
#include "Core/Time.h"

#include "Data/Reflection.h"
#include "Data/BinarySerialization.h"
#include "Data/TextSerialization.h"
#include "Data/DynamicSerializer.h"
#include "Data/Variable.h"
#include "Data/ValueType.h"

#include "Graphics/OpenGL/GLEnum.h"
#include "Graphics/OpenGL/GLExtensions.h"
#include "Graphics/OpenGL/GLSL.h"
#include "Graphics/Caps.h"
#include "Graphics/ObjectDescs.h"
#include "Graphics/States.h"
#include "Graphics/UniformType.h"

#include "GUI/FontLoadingDeclarations.h"
#include "GUI/FontLoading.h"
#include "GUI/GraphicsWindow.h"
#include "GUI/MessageBox.h"

#include "Imaging/Image.h"
#include "Imaging/ImagingTypes.h"
#include "Imaging/Bindings/DXGI_Formats.h"
#include "Imaging/Bindings/GLenumFormats.h"

#include "IO/DocumentWriter.h"
#include "IO/File.h"
#include "IO/LogSystem.h"
#include "IO/StaticStream.h"
#include "IO/Stream.h"

#include "Math/Bit.h"
#include "Math/Fixed.h"
#include "Math/LinAl.h"
#include "Math/MathEx.h"
#include "Math/Matrix.h"
#include "Math/Quaternion.h"
#include "Math/Shapes.h"
#include "Math/Simd.h"
#include "Math/Vector.h"
#include "Math/Random.h"

#include "Memory/Memory.h"
#include "Memory/Allocator.h"
#include "Memory/SystemAllocators.h"
#include "Memory/VirtualMemory.h"
#include "Memory/AllocatorInterface.h"
#include "Memory/SmartRef.h"

#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/Mixins.h"
#include "Meta/Preprocessor.h"
#include "Meta/TypeList.h"

#include "Platform/PlatformInfo.h"
#include "Platform/Endianess.h"
#include "Platform/HardwareInfo.h"
#include "Platform/Platform.h"

#include "Sound/SoundTypes.h"
#include "Sound/Sound.h"
#include "Sound/Music.h"
#include "Sound/SoundBuilder.h"
#include "Sound/SoundProcessing.h"
#include "Sound/SoundSource.h"
#include "Sound/SynthesizedInstrument.h"
#include "Sound/InstrumentLibrary.h"

#include "Test/PerformanceTest.h"
#include "Test/UnitTest.h"

#include "Text/CharMap.h"
#include "Text/UtfConversion.h"

#include "Threading/Atomic.h"
#include "Threading/Job.h"
#include "Threading/Thread.h"

#include "Utils/Callback.h"
#include "Utils/Event.h"
#include "Utils/Optional.h"
