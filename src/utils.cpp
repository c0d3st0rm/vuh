#include <vuh/utils.h>
#include <vuh/error.h>
#include <vuh/arr/arrayUtils.h>

#include <fstream>

namespace vuh {
	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* filename)-> std::vector<char> {
		auto fin = std::ifstream(filename, std::ios::binary);
		if(!fin.is_open()){
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			VULKAN_HPP_ASSERT(0);
			return std::vector<char>();
#else
			throw FileReadFailure(std::string("could not open file ") + filename + " for reading");
#endif
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

namespace arr {
	/// Copy data between device buffers using the device transfer command pool and queue.
	/// Source and destination buffers are supposed to be allocated on the same device.
	/// Fully sync, no latency hiding whatsoever.
	auto copyBuf(vuh::Device& device ///< device where buffers are allocated
	             , VULKAN_HPP_NAMESPACE::Buffer src    ///< source buffer
	             , VULKAN_HPP_NAMESPACE::Buffer dst    ///< destination buffer
	             , size_t size_bytes ///< size of memory chunk to copy in bytes
	             , size_t src_offset ///< source buffer offset (bytes)
	             , size_t dst_offset ///< destination buffer offset (bytes)
	             )-> void
	{
		auto cmd_buf = device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(src_offset, dst_offset, size_bytes);
		cmd_buf.copyBuffer(src, dst, 1, &region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
		queue.waitIdle();
	}

	auto copyImageToBuffer(vuh::Device& device
			, VULKAN_HPP_NAMESPACE::Image src
			, VULKAN_HPP_NAMESPACE::Buffer dst
			, uint32_t imageWidth
			, uint32_t imageHeight
			, size_t bufferOffset
	)-> void
	{
		auto cmd_buf = device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		VULKAN_HPP_NAMESPACE::BufferImageCopy region;
		region.setBufferOffset(bufferOffset);
		region.setImageExtent({imageWidth, imageHeight, 1});
		cmd_buf.copyImageToBuffer(src, VULKAN_HPP_NAMESPACE::ImageLayout::eTransferSrcOptimal, dst, 1, &region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
		queue.waitIdle();
	}

	auto copyBufferToImage(vuh::Device& device
			, VULKAN_HPP_NAMESPACE::Buffer src
			, VULKAN_HPP_NAMESPACE::Image dst
			, uint32_t imageWidth
			, uint32_t imageHeight
			, size_t bufferOffset
	)-> void
	{
		auto cmd_buf = device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		VULKAN_HPP_NAMESPACE::BufferImageCopy region;
		region.setBufferOffset(bufferOffset);
		region.setImageExtent({imageWidth, imageHeight, 1});
		cmd_buf.copyBufferToImage(src, dst, VULKAN_HPP_NAMESPACE::ImageLayout::eTransferDstOptimal, 1, &region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
		queue.waitIdle();
	}
} // namespace arr
} // namespace vuh
