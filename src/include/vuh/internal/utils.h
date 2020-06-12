#pragma once
#include <string>
#include <vector>
#include <vuh/vuh.h>

#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace {
	using std::begin; using std::end;

	/// @return true if value x can be extracted from an array with a given function
	template<class U, class F>
	auto contains(const char* x, const std::vector<U>& array, F&& fun)-> bool {
		return end(array) != std::find_if(ALL(array), [&](auto& r){return 0 == std::strcmp(x, fun(r));});
	}

	/// Extend vector of string literals by candidate values that have a macth in a reference set.
	template<class U, class T, class F>
	auto filter_list(std::vector<const char*> old_values ///< array to extend
	                 , const U& tst_values               ///< candidate values
	                 , const T& ref_values               ///< reference values
	                 , F&& ffield                        ///< maps reference values to candidate values manifold
	                 , vuh::debug_reporter_t report_cbk=nullptr ///< error reporter
	                 , const char* layer_msg=nullptr     ///< base part of the log message about unsuccessful candidate value
	                 )-> std::vector<const char*>
	{
		using std::begin;
		for(const auto& l: tst_values){
			if(contains(l, ref_values, ffield)){
				old_values.push_back(l);
			} else {
				if(report_cbk != nullptr){
					auto msg = std::string("value ") + l + " is missing";
					report_cbk({}, {}, 0, 0, 0, layer_msg, msg.c_str(), nullptr);
				}
			}
		}
		return old_values;
	}

	template<class Ex, class U>
	auto find_missing_and_throw(const std::vector<const char*>& V, const std::vector<U>& E)->void {
		// find the layer/extension that is missing
		for (const auto& v : V) {
			if (!contains(v, E, [](const char* s) { return s; })) {
				throw Ex(v);
			}
		}
	}
}