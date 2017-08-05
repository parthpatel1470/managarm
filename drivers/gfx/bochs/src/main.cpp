
#include <assert.h>
#include <stdio.h>
#include <deque>
#include <experimental/optional>
#include <functional>
#include <iostream>
#include <memory>

#include <arch/bits.hpp>
#include <arch/register.hpp>
#include <arch/io_space.hpp>
#include <async/result.hpp>
#include <boost/intrusive/list.hpp>
#include <cofiber.hpp>
#include <frigg/atomic.hpp>
#include <frigg/arch_x86/machine.hpp>
#include <frigg/memory.hpp>
#include <helix/ipc.hpp>
#include <helix/await.hpp>
#include <protocols/fs/server.hpp>
#include <protocols/hw/client.hpp>
#include <protocols/mbus/client.hpp>
#include <protocols/usb/usb.hpp>
#include <protocols/usb/api.hpp>
#include <protocols/usb/server.hpp>

#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

#include "bochs.hpp"
#include <fs.pb.h>

// ----------------------------------------------------------------
// Stuff that belongs in a DRM library.
// ----------------------------------------------------------------

async::result<int64_t> DrmDevice::seek(std::shared_ptr<void> object, int64_t offset) {
	throw std::runtime_error("seek() not implemented");
}

async::result<size_t> DrmDevice::read(std::shared_ptr<void> object, void *buffer, size_t length) {
	throw std::runtime_error("read() not implemented");
}

async::result<void> DrmDevice::write(std::shared_ptr<void> object, const void *buffer, size_t length) {
	throw std::runtime_error("write() not implemented");
}

async::result<helix::BorrowedDescriptor> DrmDevice::accessMemory(std::shared_ptr<void> object) {
	throw std::runtime_error("accessMemory() not implemented");
}

COFIBER_ROUTINE(async::result<void>, DrmDevice::ioctl(std::shared_ptr<void> object, managarm::fs::CntRequest req,
		helix::UniqueLane conversation), ([object = std::move(object), req = std::move(req),
		conversation = std::move(conversation)] {
	if(req.command() == DRM_IOCTL_GET_CAP) {
		helix::SendBuffer send_resp;
		managarm::fs::SvrResponse resp;

		if(req.drm_capability() == DRM_CAP_DUMB_BUFFER) {
			resp.set_drm_value(1);
			resp.set_error(managarm::fs::Errors::SUCCESS);
		}else{
			resp.set_drm_value(0);
			resp.set_error(managarm::fs::Errors::ILLEGAL_ARGUMENT);
		}
		
		auto ser = resp.SerializeAsString();
		auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
			helix::action(&send_resp, ser.data(), ser.size()));
		COFIBER_AWAIT transmit.async_wait();
		HEL_CHECK(send_resp.error());
	}else if(req.command() == DRM_IOCTL_MODE_GETRESOURCES) {
		helix::SendBuffer send_resp;
		managarm::fs::SvrResponse resp;

		resp.add_drm_crtc_ids(uint32_t(1));
		resp.add_drm_connector_ids(uint32_t(2));
		resp.add_drm_encoder_ids(uint32_t(3));
		resp.set_drm_min_width(640);
		resp.set_drm_max_width(1024);
		resp.set_drm_min_height(480);
		resp.set_drm_max_height(758);
		resp.set_error(managarm::fs::Errors::SUCCESS);
	
		auto ser = resp.SerializeAsString();
		auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
			helix::action(&send_resp, ser.data(), ser.size()));
		COFIBER_AWAIT transmit.async_wait();
		HEL_CHECK(send_resp.error());
	}else if(req.command() == DRM_IOCTL_MODE_GETCONNECTOR) {
		helix::SendBuffer send_resp;
		managarm::fs::SvrResponse resp;
		
		resp.add_drm_encoders(3);
		auto mode = resp.add_drm_modes();
		mode->set_clock(47185);
		mode->set_hdisplay(1024);
		mode->set_hsync_start(1024);
		mode->set_hsync_end(1024);
		mode->set_htotal(1024);
		mode->set_hskew(0);
		mode->set_vdisplay(768);
		mode->set_vsync_start(768);
		mode->set_vsync_end(768);
		mode->set_vtotal(768);
		mode->set_vscan(0);
		mode->set_vrefresh(60);
		mode->set_flags(0);
		mode->set_type(0);
		mode->set_name("1024x768");
		resp.set_drm_encoder_id(0);
		resp.set_drm_connector_type(0);
		resp.set_drm_connector_type_id(0);
		resp.set_drm_connection(1); // DRM_MODE_CONNECTED
		resp.set_drm_mm_width(306);
		resp.set_drm_mm_height(230);
		resp.set_drm_subpixel(0);
		resp.set_error(managarm::fs::Errors::SUCCESS);
	
		auto ser = resp.SerializeAsString();
		auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
			helix::action(&send_resp, ser.data(), ser.size()));
		COFIBER_AWAIT transmit.async_wait();
		HEL_CHECK(send_resp.error());
	}else if(req.command() == DRM_IOCTL_MODE_GETENCODER) {
		helix::SendBuffer send_resp;
		managarm::fs::SvrResponse resp;
	
		resp.set_drm_encoder_type(0);
		resp.set_drm_crtc_id(1);
		resp.set_drm_possible_crtcs(1);
		resp.set_drm_possible_clones(0);
		resp.set_error(managarm::fs::Errors::SUCCESS);
	
		auto ser = resp.SerializeAsString();
		auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
			helix::action(&send_resp, ser.data(), ser.size()));
		COFIBER_AWAIT transmit.async_wait();
		HEL_CHECK(send_resp.error());
	}else if(req.command() == DRM_IOCTL_MODE_CREATE_DUMB) {
		helix::SendBuffer send_resp;
		managarm::fs::SvrResponse resp;
	
		resp.set_drm_handle(1);
		resp.set_drm_pitch(1024 * 4);
		resp.set_drm_size(1024 * 768 * 4);
		resp.set_error(managarm::fs::Errors::SUCCESS);
	
		auto ser = resp.SerializeAsString();
		auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
			helix::action(&send_resp, ser.data(), ser.size()));
		COFIBER_AWAIT transmit.async_wait();
		HEL_CHECK(send_resp.error());
	}else{
		throw std::runtime_error("Unknown ioctl() with ID" + std::to_string(req.command()));
	}
	COFIBER_RETURN();
}))

constexpr protocols::fs::FileOperations fileOperations {
	&DrmDevice::seek,
	&DrmDevice::seek,
	&DrmDevice::seek,
	&DrmDevice::read,
	&DrmDevice::write,
	&DrmDevice::accessMemory,
	&DrmDevice::ioctl
};

COFIBER_ROUTINE(cofiber::no_future, serveDevice(std::shared_ptr<DrmDevice> device,
		helix::UniqueLane p), ([device = std::move(device), lane = std::move(p)] {
	std::cout << "unix device: Connection" << std::endl;

	while(true) {
		helix::Accept accept;
		helix::RecvInline recv_req;

		auto &&header = helix::submitAsync(lane, helix::Dispatcher::global(),
				helix::action(&accept, kHelItemAncillary),
				helix::action(&recv_req));
		COFIBER_AWAIT header.async_wait();
		HEL_CHECK(accept.error());
		HEL_CHECK(recv_req.error());
		
		auto conversation = accept.descriptor();
		managarm::fs::CntRequest req;
		req.ParseFromArray(recv_req.data(), recv_req.length());
		if(req.req_type() == managarm::fs::CntReqType::DEV_OPEN) {
			helix::SendBuffer send_resp;
			helix::PushDescriptor push_node;
			
			helix::UniqueLane local_lane, remote_lane;
			std::tie(local_lane, remote_lane) = helix::createStream();
			protocols::fs::servePassthrough(std::move(local_lane), device,
					&fileOperations);

			managarm::fs::SvrResponse resp;
			resp.set_error(managarm::fs::Errors::SUCCESS);

			auto ser = resp.SerializeAsString();
			auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
					helix::action(&send_resp, ser.data(), ser.size(), kHelItemChain),
					helix::action(&push_node, remote_lane));
			COFIBER_AWAIT transmit.async_wait();
			HEL_CHECK(send_resp.error());
			HEL_CHECK(push_node.error());
		}else{
			throw std::runtime_error("Invalid request in serveDevice()");
		}
	}
}))

// ----------------------------------------------------------------

// ----------------------------------------------------------------
// Helper Funcs.
// ----------------------------------------------------------------

void fillBuffer(void* frame_buffer, int width, int height){
	char* ptr = (char*)frame_buffer;
	for(int j = 0; j < height; j++) {
		for(int i = 0; i < width; i++) {
			// RGBX
			ptr[4 * (j * width + i) + 0] = 0xCE;
			ptr[4 * (j * width + i) + 1] = 0x13;
			ptr[4 * (j * width + i) + 2] = 0x95;
			ptr[4 * (j * width + i) + 3] = 0x00;
		}
	}
};

// ----------------------------------------------------------------
// GfxDevice.
// ----------------------------------------------------------------

GfxDevice::GfxDevice(void* frame_buffer)
: _frameBuffer{frame_buffer} {
	uintptr_t ports[] = { 0x01CE, 0x01CF, 0x01D0 };
	HelHandle handle;
	HEL_CHECK(helAccessIo(ports, 3, &handle));
	HEL_CHECK(helEnableIo(handle));
	
	_operational = arch::global_io;
}

COFIBER_ROUTINE(cofiber::no_future, GfxDevice::initialize(), ([=] {
	_operational.store(regs::index, (uint16_t)RegisterIndex::id);
	auto version = _operational.load(regs::data); 
	if(version < 0xB0C2) {
		std::cout << "gfx/bochs: Device version 0x" << std::hex << version << std::dec
				<< " may be unsupported!" << std::endl;
	}

	_operational.store(regs::index, (uint16_t)RegisterIndex::enable);
	_operational.store(regs::data, enable_bits::noMemClear || enable_bits::lfb);
	
	_operational.store(regs::index, (uint16_t)RegisterIndex::virtWidth);
	_operational.store(regs::data, 1024);
	_operational.store(regs::index, (uint16_t)RegisterIndex::virtHeight);
	_operational.store(regs::data, 768);
	_operational.store(regs::index, (uint16_t)RegisterIndex::bpp);
	_operational.store(regs::data, 32);

	_operational.store(regs::index, (uint16_t)RegisterIndex::resX);
	_operational.store(regs::data, 1024);
	_operational.store(regs::index, (uint16_t)RegisterIndex::resY);
	_operational.store(regs::data, 768);
	_operational.store(regs::index, (uint16_t)RegisterIndex::offX);
	_operational.store(regs::data, 0);
	_operational.store(regs::index, (uint16_t)RegisterIndex::offY);
	_operational.store(regs::data, 0);
	
	_operational.store(regs::index, (uint16_t)RegisterIndex::enable);
	_operational.store(regs::data, enable_bits::enable 
			|| enable_bits::noMemClear || enable_bits::lfb);
	
	fillBuffer(_frameBuffer, 1024, 768);
}))

// ----------------------------------------------------------------
// Freestanding PCI discovery functions.
// ----------------------------------------------------------------

COFIBER_ROUTINE(cofiber::no_future, bindController(mbus::Entity entity), ([=] {
	protocols::hw::Device pci_device(COFIBER_AWAIT entity.bind());
	auto info = COFIBER_AWAIT pci_device.getPciInfo();
	assert(info.barInfo[0].ioType == protocols::hw::IoType::kIoTypeMemory);
	auto bar = COFIBER_AWAIT pci_device.accessBar(0);
	
	void *actual_pointer;
	HEL_CHECK(helMapMemory(bar.getHandle(), kHelNullHandle, nullptr,
			0, info.barInfo[0].length, kHelMapReadWrite | kHelMapShareAtFork, &actual_pointer));

	auto gfx_device = std::make_shared<GfxDevice>(actual_pointer);
	gfx_device->initialize();
	
	// Create an mbus object for the device.
	auto root = COFIBER_AWAIT mbus::Instance::global().getRoot();
	
	std::unordered_map<std::string, std::string> descriptor {
		{ "unix.devtype", "block" },
		{ "unix.devname", "card0" },
	};
	auto object = COFIBER_AWAIT root.createObject("gfx_bochs", descriptor,
			[=] (mbus::AnyQuery query) -> async::result<helix::UniqueDescriptor> {
		helix::UniqueLane local_lane, remote_lane;
		std::tie(local_lane, remote_lane) = helix::createStream();
		serveDevice(gfx_device, std::move(local_lane));

		async::promise<helix::UniqueDescriptor> promise;
		promise.set_value(std::move(remote_lane));
		return promise.async_get();
	});
}))

COFIBER_ROUTINE(cofiber::no_future, observeControllers(), ([] {
	auto root = COFIBER_AWAIT mbus::Instance::global().getRoot();

	auto filter = mbus::Conjunction({
		mbus::EqualsFilter("pci-vendor", "1234")
	});
	COFIBER_AWAIT root.linkObserver(std::move(filter),
			[] (mbus::AnyEvent event) {
		if(event.type() == typeid(mbus::AttachEvent)) {
			std::cout << "gfx/bochs: Detected device" << std::endl;
			bindController(boost::get<mbus::AttachEvent>(event).getEntity());
		}else{
			throw std::runtime_error("Unexpected event type");
		}
	});
}))

int main() {
	printf("gfx/bochs: Starting driver\n");
	
	observeControllers();

	while(true)
		helix::Dispatcher::global().dispatch();
	
	return 0;
}

