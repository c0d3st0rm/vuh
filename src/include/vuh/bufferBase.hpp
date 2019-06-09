#pragma once

#include "allocatorDevice.hpp"
#include "device.hpp"
#include "physicalDevice.hpp"

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstdint>

namespace vuh {
/// Wraps the SBO buffer. Covers base buffer functionality.
/// Keeps the data, handles initialization, copy/move, common interface,
/// binding memory to buffer objects, etc...
template<class Alloc>
class BufferBase {
	static constexpr auto descriptor_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
public:
	static constexpr auto descriptor_class = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	/// Construct SBO array of given size in device memory
	BufferBase(const Device& device                  ///< device to allocate array
	           , std::size_t size_bytes              ///< size in bytes
	           , VkMemoryPropertyFlags properties={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
	           , VkBufferUsageFlags usage={}         ///< additional usage flags. These are 'added' to flags defined by allocator.
	           )
	   : _buffer(Alloc::makeBuffer(device, size_bytes, descriptor_flags | usage))
	   , _device(device)
	{
		auto device_memory = Alloc::allocMemory(device, _buffer, properties);
		VUH_CHECKOUT();
		_mem = device_memory.memory;
		auto memory_properties = VkPhysicalDeviceMemoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(device.physical(), &memory_properties);
		_flags = memory_properties.memoryTypes[device_memory.id].propertyFlags;
		auto err = vkBindBufferMemory(device, _buffer, _mem, 0);
		if(err != VK_SUCCESS){
			release();
			VUH_SIGNAL(err);
		}
	}

	/// Release resources associated with current object.
	~BufferBase() noexcept {release();}
   
	BufferBase(const BufferBase&)=delete;
	auto operator= (const BufferBase&)->BufferBase& = delete;

	/// Move constructor. Passes the underlying buffer ownership.
	BufferBase(BufferBase&& other) noexcept
	   : _buffer(other._buffer), _mem(other._mem), _flags(other._flags), _device(other._device)
	{
		other._buffer = nullptr;
	}

	auto operator= (BufferBase&& other) noexcept-> BufferBase& {
		assert(_device == other._device);
		std::swap(_buffer, other._buffer);
		std::swap(_mem, other._mem);
		std::swap(_flags, other._flags);
		return *this;
	}

	/// @return underlying buffer
	auto buffer() const-> VkBuffer { return _buffer; }
	operator VkBuffer() const {return _buffer;}

	/// @return offset (in bytes) of the current buffer from the beginning of associated device memory.
	/// For arrays managing their own memory this is always 0.
	constexpr auto offset() const-> std::size_t { return 0u; }
	constexpr auto offset_bytes() const-> std::size_t { return 0u; }

	/// @return reference to device on which underlying buffer is allocated
	auto device() const-> const Device& {return _device;}

	auto memory() const-> VkDeviceMemory {return _mem;}

	/// @return true if array is host-visible, ie can expose its data via a normal host pointer.
	auto host_visible() const-> bool {
		return (_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
	}
protected: // helpers
	/// release resources associated with current BufferBase object
	auto release() noexcept-> void {
		if(_buffer){
			vkFreeMemory(_device, _mem, nullptr);
			vkDestroyBuffer(_device, _buffer, nullptr);
			_buffer = nullptr;
		}
	}
protected: // data
	VkBuffer _buffer;              ///< doc me
	VkDeviceMemory _mem;           ///< associated chunk of device memory
	VkMemoryPropertyFlags _flags;  ///< actual flags of allocated memory (may differ from those requested)
	const Device& _device;         ///< referes underlying logical device
}; // class BufferBase

/// non-owning data view
template<class T>
class HostDataView {
public:
	using value_type = T;
	using pointer = T*;

	explicit HostDataView(T* data, std::size_t count): _data(data), _size(count){}

	auto data() const-> T* {return _data;}
	auto begin() const-> T* {return _data;}
	auto end() const-> T* {return _data + _size;}
	auto size() const-> std::size_t {return _size;}
	auto size_bytes() const-> std::size_t {return _size*sizeof(value_type);}
	auto operator[](std::size_t off) const->T& {return *(_data + off);}
private:
	T* _data;
	std::size_t _size; ///< element count
}; // class HostDataView

///
template<class T, class Buf>
class HostData: public HostDataView<T> {
	using Base = HostDataView<T>;
public:
	using value_type = typename Base::value_type;
	using pointer = typename Base::pointer;

	explicit HostData(Buf& buffer, std::size_t count)
	   : Base(map_memory(buffer, count*sizeof(T)), count), _buffer(&buffer)
	{}

	~HostData() noexcept {
		if(_buffer){
			vkUnmapMemory(_buffer->device(), _buffer->memory());
		}
	}

	HostData(const HostData&)=delete;
	auto operator= (const HostData&)->HostData& = delete;

	HostData(HostData&& other) noexcept
	   : Base(static_cast<Base&&>(other)), _buffer(other._buffer)
	{
		other._buffer = nullptr;
	}
	auto operator=(HostData&& other) noexcept-> HostData& {
		std::swap(_buffer, other._buffer);
		return *this;
	}

	static auto map_memory(Buf& buffer, std::size_t size_bytes)-> T* {
		auto ret = pointer{};
		VUH_CHECK(vkMapMemory(buffer.device(), buffer.memory(), 0, size_bytes, {}, (void**)(&ret)));
		return ret;
	}
	operator HostData<const value_type, Buf>&() {return *this;}
protected: // data
	Buf* _buffer; ///< doc me. nonnull
}; // class HostData

} // namespace vuh
