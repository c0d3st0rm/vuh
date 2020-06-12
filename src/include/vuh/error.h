#pragma once

#include <vulkan/vulkan.hpp>

namespace vuh {
	/// Exception indicating that memory with requested properties
	/// has not been found on a device.
	class NoSuitableMemoryFound: public vk::OutOfDeviceMemoryError {
	public:
		NoSuitableMemoryFound(const std::string& message);
		NoSuitableMemoryFound(const char* message);
	};

	/// Exception indicating failure to read a file.
	class FileReadFailure: public std::runtime_error {
	public:
		FileReadFailure(const std::string& message);
		FileReadFailure(const char* message);
	};

	// Exception indicating a mandatory requested layer was not found
	class LayerNotFound: public std::runtime_error {
	public:
		LayerNotFound(const std::string& message);
		LayerNotFound(const char* message);
	};

	// Exception indicating a mandatory requested extension was not found
	class ExtensionNotFound: public std::runtime_error {
	public:
		ExtensionNotFound(const std::string& message);
		ExtensionNotFound(const char* message);
	};
} // namespace vuh
