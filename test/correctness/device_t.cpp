#include <catch2/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <iostream>

using std::begin;
using std::end;

namespace {
	auto instance = vuh::Instance();
} // namespace

TEST_CASE("initialization & system info", "[correctness]"){
	SECTION("physical devices properties and queues"){
		for(const auto& pd: instance.physicalDevices()){
			auto props = pd.getProperties(); // forwards vk::PhysicalDevice

			std::cout << "physical device: " << pd.name() << "\n"
			          << "\t compute enabled queues: " << pd.numComputeQueues() << "\n"
			          << "\t transfer enabled queues: " << pd.numTransferQueues() << "\n"
			          << "\t multi-purpose queues: " << pd.numMixedQueues() << "\n";

			auto qfms = pd.queueFamilies();
			for(const auto& qf: qfms){
				std::cout << "family " << qf.id()
				          << "supports compute: " << (qf.canCompute() ? "yes" : "no")
				          << "supports transfer: " << (qf.canTransfer() ? "yes" : "no")
				          << "has queues: " << qf.numQueues()
				          << "flags: " << qf.flags() << "\n";
			}
		}
	}
	SECTION("wild ideas"){
		auto pd = instance.physicalDevices().at(0);
		auto dev0 = pd.computeDevice(); // default, best allocation of 1 transport and 1 compute queue
		auto dev1 = pd.computeDevice(vuh::Queues::Default); // same default
		auto dev2 = pd.computeDevice(vuh::Queues::All);     // claim all queues
		auto dev3 = pd.computeDevice(vuh::Queues::Streams(4)); // best possible allocation of queues to accomodate 4 streams
		auto dev4 = pd.computeDevice(vuh::Queues::Spec
		                             { {0 /*family_id*/, 4 /*number queues*/, {/*priorities*/}}
		                             , {1, 2, {}}
		                             , {2, 1, {}} });
		for(size_t i = 0; i < dev4.nQueues(); ++i){
			auto q_i = dev4.queue(i);
			std::cout << q_i.canCompute() << q_i.canTransfer() << "\n";
		}
	}
	SECTION("default compute device"){
		auto phys_dev = instance.physicalDevices().at(0);
		auto dev = phys_dev.computeDevice(); // only default queues
		SECTION("attach queues to a default-constructed device"){
			SECTION("compute and transfer queues"){
				dev.attachQueues( ComputeQueues(phys_dev.numComputeQueues())
				                , TransferQueues(phys_dev.numTransferQueues())
				                );
				REQUIRE(dev.numComputeQueues() == phys_dev.numComputeQueues());
				REQUIRE(dev.numTransferQueues() == phys_dev.numTransferQueues());
			}
			SECTION("attach mixed queues to a default-constructed device"){
				dev.attachQueues(MixedQueues(phys_dev.numMixedQueues()));
				REQUIRE(dev.numMixedQueues() == phys_dev.numMixedQueues());
			}
			SECTION("claiming more queues then physically available throws"){
				dev.attachQueues(ComputeQueues(phys_dev.numComputeQueues())
				                , TransferQueues(phys_dev.numTransferQueues)
				                , MixedQueues(phys_dev.numMixedQueues())); // this throws when there is at least one mixed capability queue
				
				dev.attachQueues(ComputeQueues(phys_dev.numComputeQueues() + 1));
				dev.attachQueues(TransferQueues(phys_dev.numTransferQueues() + 1));
				dev.attachQueues(MixedQueues(phys_dev.numMixedQueues() + 1));
			}
		}
		SECTION("fine-grained queues creation"){
			auto qfms = phys_dev.queueFamilies();
			SECTION("queues specs as a vector"){
				auto queue_specs = std::vector<vuh::QueueSpecs>{};
				for(auto qf: qfms) {
					if(qf.canCompute()){
						for(size_t qid = 0; qid < qf.numQueues(); ++qid){
							queue_specs.push_back({qf.id(), qid}); // individual queues selection
						}
					} else if(qf.canTransfer()){
						queue_specs.push_back({qf.id(), 0, qf.numQueues()}); // specify queue id range
					}
				}
				dev.attachQueues(queue_specs);
				REQUIRE(dev.numComputeQueues() == phys_dev.numComputeQueues());
				REQUIRE(dev.numTransferQueues() == phys_dev.numTranferQueues());
			}
			SECTION("queues specs as initializer list"){
				const auto id_compute = 2;  // id of the queue family with compute support
				const auto id_transfer = 1; // is of the queue family with transfer support
				dev.attachQueues({{id_transfer, 0}, {id_compute, 0, 3}});
			}
		}
	}
}
